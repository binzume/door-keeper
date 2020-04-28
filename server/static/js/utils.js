"use strict";

function getxhr() {
	var xhr;
	if(window.XMLHttpRequest) {
		xhr =  new XMLHttpRequest();
	} else if(window.ActiveXObject) {
		try {
			xhr = new ActiveXObject('Msxml2.XMLHTTP');
		} catch (e) {
			xhr = new ActiveXObject('Microsoft.XMLHTTP');
		}
	}
	return xhr;
}

function getJson(url,f){
	var xhr = getxhr();
	xhr.open('GET', url);
	xhr.onreadystatechange = function() {
		if (xhr.readyState != 4) return;
		if (f) {
			if (xhr.status == 200) {
				f(JSON.parse(xhr.responseText))
			} else {
				f(undefined)
			}
		}
	};
	xhr.send();
}

function requestJson(method, url, f){
	var xhr = getxhr();
	xhr.open(method, url);
	xhr.onreadystatechange = function() {
		if (xhr.readyState != 4) return;
		if (f) {
			if (xhr.status == 200) {
				f(JSON.parse(xhr.responseText))
			} else {
				f(undefined)
			}
		}
	};
	return xhr;
}


function element_append(e, value) {
	if (value instanceof Array) {
		for (var i = 0; i < value.length; i++) {
			element_append(e, value[i]);
		}
		return;
	}
	if (typeof value == 'string') {
		value = document.createTextNode(value);
	}
	e.appendChild(value);
}

function element_clear(e) {
	while (e.firstChild) {
	    e.removeChild(e.firstChild);
	}
}

function element(tag, children, attr) {
	var e = document.createElement(tag);
	if (children) {
		element_append(e, children);
	}
	if (typeof(attr) == "function") {
		attr(e);
	} else if (typeof(attr) == "object") {
		for (var key in attr) {
			e[key] = attr[key];
		}
	}
	return e;
}

function bindObj(o, f) {
	return function() {return f.apply(o, arguments)};
}

if (window.$ === undefined) {
	window.$ = document.querySelector.bind(document);
}

