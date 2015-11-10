package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net"
	"net/http"
)

var devs = map[string]*net.UDPAddr{}
var conn2 *net.UDPConn
var apiToken = "********"

func udpsrv() {
	addr, err := net.ResolveUDPAddr("udp", "0.0.0.0:9000")
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
		if m["token"] == apiToken {
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
	if r.FormValue("token") != apiToken {
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
	_, err := conn2.WriteToUDP([]byte("token:"+apiToken+"\tcommand:"+command), devs[dev])
	if err != nil {
		log.Fatalf("%v\n", err)
	}

	json, _ := json.Marshal(map[string]string{"status": "ok"})
	fmt.Fprintf(w, string(json))
}

func main() {
	go udpsrv()
	log.Print("Start http")
	http.HandleFunc("/", index)
	http.HandleFunc("/status", status)
	http.HandleFunc("/send", send)
	http.ListenAndServe("0.0.0.0:9000", nil)
}
