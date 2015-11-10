package main

import (
	"encoding/json"
	"fmt"
	"log"
        "net"
	"net/http"
)

func udpsrv() {
	addr, err := net.ResolveUDPAddr("udp", "0.0.0.0:9000")
        if err != nil {
                log.Fatalln(err)
        }

        conn, err := net.ListenUDP("udp", addr)

        if err != nil {
                log.Fatalln(err)
        }

        defer conn.Close()

        buf := make([]byte, 128)

        for {
                rlen, remote, err := conn.ReadFromUDP(buf)

                if err != nil {
                        log.Fatalf("Error: %v\n", err)
                }

                s := string(buf[:rlen])

                log.Printf("Receive [%v]: %v\n", remote, s)
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

func main() {
	go udpsrv()
	log.Print("Start http")
	http.HandleFunc("/", index)
	http.HandleFunc("/status", status)
	http.ListenAndServe("0.0.0.0:9000", nil)
}

