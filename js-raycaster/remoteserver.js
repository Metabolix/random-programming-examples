function RemoteServer(address, name) {
	if (name.match(/[^-_.0-9A-Za-zåäöÅÄÖ]/)) {
		throw "Invalid name!";
	}
	this.version = "2024-04-06"
	this.connected = false;
	this.disconnected = false;
	this.address = address;
	this.name = this.name2 = name;
	this.socket = null;
	this.connect();
}

(function() {
	var e;
	try {
		if (WebSocket.CONNECTING !== 0) throw null;
		RemoteServer.supported = true;
	} catch (e) {
		RemoteServer.supported = false;
	}
})();

RemoteServer.prototype.connect = function() {
	var _this = this;
	var socket = new WebSocket(this.address);
	socket.onopen = function() { _this.open(socket); };
	socket.onclose = function() { _this.close(socket); };
	socket.onmessage = function(e) { _this.message(socket, e.data); };
	socket.onerror = function(e) { _this.error(socket, e.data); };
}

RemoteServer.prototype.open = function(socket) {
	if (this.socket) {
		return;
	}
	this.socket = socket;
	this.socket.send(this.version);
}

RemoteServer.prototype.message = function(socket, message) {
	if (socket != this.socket) {
		return;
	}
	var m;
	if (message.match(/^QUIT/)) {
		this.disconnect();
		return;
	}
	if (message.match(/^NAME/)) {
		this.name = this.name2;
		this.socket.send("NAME:" + this.name);
		this.name2 += "_";
		return;
	}
	if (message.match(/^LEVEL;/)) {
		level.parse(message.substr(6));
		timer.reset();
		return;
	}
	if (message.match(/^PLAYERS;/)) {
		players.set(this.parsePlayers(message.substr(8)), this.name);
		return;
	}
	if (message.match(/^BALL;/)) {
		var m = message.substr(5).split(":");
		balls.add(parseInt(m[0]), {
			"pos": {
				"x": parseFloat(m[1]),
				"y": parseFloat(m[2])
			},
			"speed": {
				"x": parseFloat(m[3]),
				"y": parseFloat(m[4])
			}
		});
		return;
	}
	if (message.match(/^EXPLODE;/)) {
		var m = message.substr(8).split(":");
		balls.remove(parseInt(m[0]));
		explosions.add({
			"x": parseFloat(m[1]),
			"y": parseFloat(m[2])
		});
		return;
	}
}

RemoteServer.prototype.parsePlayers = function(str) {
	var infos = [];
	var arr = str.split(";");
	for (var i = 0; i < arr.length; ++i) {
		var info = arr[i].split(":");
		info = {
			"pos": {
				x: parseFloat(info[0]),
				y: parseFloat(info[1])
			},
			"dir": parseFloat(info[2]),
			"keys": {
				"up": (info[3] != "0"),
				"down": (info[4] != "0"),
				"left": (info[5] != "0"),
				"right": (info[6] != "0"),
				"shoot": (info[7] != "0")
			},
			"name": info[8],
			"score": parseInt(info[9]),
			"hp": parseInt(info[10])
		};
		infos.push(info);
	}
	return infos;
};

RemoteServer.prototype.error = function(socket, error) {
	if (socket != this.socket) {
		return;
	}
	alert(error);
	try {
		this.socket.close();
	} catch (error) {
	}
	this.socket = null;
	var _this = this;
	setTimeout(function() { _this.connect(); }, 1000);
}

RemoteServer.prototype.close = function(socket) {
	if (socket != this.socket) {
		return;
	}
	this.socket = null;
}

RemoteServer.prototype.disconnect = function() {
	if (this.socket) {
		this.socket.close();
	}
}

RemoteServer.prototype.send = function(input) {
	var e;
	if (this.socket && this.socket.bufferedAmount == 0) try {
		var message = [input.up ?1:0, input.down ?1:0, input.left ?1:0, input.right ?1:0, input.shoot ?1:0].join(":");
		this.socket.send(message);
	} catch (e) {
		alert(e);
	}
}
