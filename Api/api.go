package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net"
	"net/http"
	"io/ioutil"
)

var devs = map[string]*net.UDPAddr{}
var conn2 *net.UDPConn

type Config struct {
	HttpPort int  `json:"http_port"`
	UdpPort int    `json:"udp_port"`
	UpdateToken string  `json:"update_token"`
	ApiToken string    `json:"api_token"`
}

type Device struct {
	Type string
	Ident string
	Updated	int64
	Data map[string]string
}

var config = Config{}

func udpsrv(port int) {
	addr, err := net.ResolveUDPAddr("udp", "0.0.0.0:" + fmt.Sprint(port))
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

	buf := make([]byte, 128)

	for {
		rlen, remote, err := conn.ReadFromUDP(buf)

		if err != nil {
			log.Fatalf("Error: %v\n", err)
		}

		s := string(buf[:rlen])

		log.Printf("Receive [%v]: %v\n", remote, s)

		m := map[string]string{}
		json.Unmarshal(buf[:rlen], &m)
		if m["token"] == config.UpdateToken {
			devs[m["device"]] = remote
			log.Printf("Update device=%v \n", m["device"])
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

	// FIXME
	_, err := conn2.WriteToUDP([]byte("token:"+config.UpdateToken+"\tcommand:"+command), devs[dev])
	if err != nil {
		log.Fatalf("%v\n", err)
	}

	json, _ := json.Marshal(map[string]string{"status": "ok"})
	fmt.Fprintf(w, string(json))
}

func main() {
	jsonData, _ := ioutil.ReadFile("config.json")
	// config := Config{}
	json.Unmarshal(jsonData, &config)
	go udpsrv(config.UdpPort)
	log.Print("Start http")
	http.HandleFunc("/", index)
	http.HandleFunc("/status", status)
	http.HandleFunc("/send", send)
	http.ListenAndServe("0.0.0.0:" + fmt.Sprint(config.HttpPort), nil)
}
