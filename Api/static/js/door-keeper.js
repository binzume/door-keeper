"use strict";

var apiUrl = "/";

window.addEventListener('DOMContentLoaded', (ev) => {
	document.forms['door'].addEventListener('submit', function (e) {
		e.preventDefault();
		document.querySelector("#status").innerText = "...";
		var xhr = requestJson("POST", apiUrl + "send", (result) => {
			console.log(result);
			document.querySelector("#status").innerText = result.status;
			localStorage.setItem("token", document.forms['door'].token.value);
		});
		var data  = new FormData();
		data.append("token", document.forms['door'].token.value);
		data.append("device", document.forms['door'].device.value);
		data.append("command", document.forms['door'].querySelector("[type=submit]:focus").value);
		xhr.send(data);
	});
	document.forms['door'].token.value = localStorage.getItem("token") || "";
	document.querySelector("#status").innerText = "Ready...";
});
