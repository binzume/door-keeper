package main

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"net"
	"net/http"
	"os"
	"time"
)

type Config struct {
	HttpPort    int    `json:"http_port"`
	UdpPort     int    `json:"udp_port"`
	UpdateToken string `json:"update_token"`
	ApiToken    string `json:"api_token"`
}

type Device struct {
	Ident   string
	Type    string
	Addr    *net.UDPAddr
	Data    map[string]string
	Updated time.Time
}

var config = Config{}
var conn2 *net.UDPConn
var devs = map[string]*Device{}

func udpsrv(port int) {
	addr, err := net.ResolveUDPAddr("udp", "0.0.0.0:"+fmt.Sprint(port))
	if err != nil {
		log.Fatalln(err)
	}

	conn, err := net.ListenUDP("udp", addr)
	conn2 = conn
	if err != nil {
		log.Fatalln(err)
	}

	defer conn.Close()
	log.Print("Start udp")

	buf := make([]byte, 1024)

	for {
		rlen, remote, err := conn.ReadFromUDP(buf)

		if err != nil {
			log.Fatalf("Error: %v\n", err)
		}

		s := string(buf[:rlen])

		// log.Printf("Receive [%v]: %v\n", remote, s)

		m := map[string]string{}
		json.Unmarshal(buf[:rlen], &m)
		if m["token"] == config.UpdateToken {
			d := Device{}
			d.Ident = m["device"]
			d.Type = m["type"]
			//d.Data["status"] = m["status"]
			d.Addr = remote
			d.Updated = time.Now()
			devs[m["device"]] = &d
			log.Printf("Update device=%v data=%v\n", d, m)
		} else {
			log.Printf("reject;  [%v]: %v\n", remote, s)
		}
		// len, err = conn.WriteToUDP([]byte(s), remote)
	}
}

func index(w http.ResponseWriter, r *http.Request) {
	json, _ := json.Marshal(map[string]string{"status": "ok", "message": "Hello!"})
	fmt.Fprintf(w, string(json))
}

func status(w http.ResponseWriter, r *http.Request) {
	json, _ := json.Marshal(map[string]string{"status": "ok"})
	fmt.Fprintf(w, string(json))
}

func send(w http.ResponseWriter, r *http.Request) {
	if r.FormValue("token") != config.ApiToken {
		fmt.Fprintf(w, "{}")
		return
	}
	dev := r.FormValue("device")
	command := r.FormValue("command")
	if devs[dev] == nil {
		fmt.Fprintf(w, "[null]")
		return
	}
	log.Println("SEND COMMAND: ", dev, command)

	// FIXME
	_, err := conn2.WriteToUDP([]byte("token:"+config.UpdateToken+"\tcommand:"+command), devs[dev].Addr)
	if err != nil {
		log.Fatalf("%v\n", err)
	}

	json, _ := json.Marshal(map[string]string{"status": "ok"})
	fmt.Fprint(w, string(json))
}

func static(w http.ResponseWriter, r *http.Request) {
	fileServer := http.StripPrefix("/web/", http.FileServer(http.Dir("static")))
	fileServer.ServeHTTP(w, r)
}

func main() {
	jsonData, _ := ioutil.ReadFile("config.json")
	json.Unmarshal(jsonData, &config)

	f, err := os.OpenFile("aiphone.log", os.O_RDWR|os.O_CREATE|os.O_APPEND, 0666)
	if err != nil {
		log.Fatalf("error opening file: %v", err)
	}
	defer f.Close()
	log.SetOutput(f)

	go udpsrv(config.UdpPort)
	log.Print("Start http")
	http.HandleFunc("/", index)
	http.HandleFunc("/status", status)
	http.HandleFunc("/send", send)
	http.HandleFunc("/web/", static)
	http.ListenAndServe("0.0.0.0:"+fmt.Sprint(config.HttpPort), nil)
}
