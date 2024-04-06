/**
 * Copyright 2010 Lauri Kenttä
 */

var update_timeout_id = null;
var fpsLabel;
var shadowBox, textureBox, mapBox, delayBox;
var spLevel, spStart;
var mpName, mpAddress, mpConnect, mpDisconnect;
var canvas, mapCanvas;
var spLevelEditing = false;

var fps = {
	"arr": [],
	"sum": 0,
	"delay": 0
};

var physicsFrameLength = 0.01;

var graphicsOptions = {
	"texture": true,
	"shadow": true,
	"map": true
};

var server;

function updateState(dt) {
	players.move(dt);
	balls.move(dt);
	explosions.move(dt);
}

function update() {
	if (!mapBox.checked && graphicsOptions.map) {
		var context = mapCanvas.getContext('2d');
		context.fillStyle = "black";
		context.fillRect(0, 0, context.canvas.width, context.canvas.height);
		context = null;
	}
	graphicsOptions.shadow = shadowBox.checked;
	graphicsOptions.texture = textureBox.checked;
	graphicsOptions.map = mapBox.checked;

	player = players.self;
	if (!player) {
		update_timeout_id = setTimeout(update, 100);
		return;
	}
	server.send(input);

	var t0 = timer.realTime;
	var phys_frames = 0;
	while (timer.nextFrame(physicsFrameLength)) {
		++phys_frames;
		updateState(physicsFrameLength);
	}

	drawWorld(canvas.getContext('2d'), graphicsOptions);
	if (graphicsOptions.map) {
		drawMap(mapCanvas.getContext('2d'), graphicsOptions);
	}

	updateFPS(timer.realTime - t0);
	update_timeout_id = setTimeout(update, fps.delay);
}

function updateFPS(dt) {
	fps.arr.push(dt);
	fps.sum += dt;
	if (fps.arr.length >= 20) {
		var str = (Math.round(10 * fps.arr.length / fps.sum) / 10).toString();
		if (str.indexOf('.') == -1) {
			str += ".0";
		}
		fpsLabel.nodeValue = str;

		while (fps.arr.length > 10) {
			fps.sum -= fps.arr[0];
			fps.arr.shift();
		}
	}
}

window.onkeydown = function(e) {
	if (spLevelEditing) return;
	input.getInput(e.keyCode, true);
};

window.onkeyup = function(e) {
	if (spLevelEditing) return;
	input.getInput(e.keyCode, false);
};

function endGame() {
	if (update_timeout_id) {
		clearTimeout(update_timeout_id);
		update_timeout_id = null;
	}
	if (server) {
		server.disconnect();
		server = null;
	}
	players.set({});
}

function storeName() {
	var e;
	try {
		localStorage.setItem("name", mpName.value);
	} catch (e) {
	}
}

function restoreName() {
	var e;
	try {
		e = localStorage.getItem("name");
		if (e) {
			mpName.value = e;
		}
	} catch (e) {
	}
}

function startGame(startServer) {
	endGame();
	storeName();
	server = startServer();
	canvas.focus();
	update();
}

function startSinglePlayer() {
	startGame(() => new LocalServer(mpName.value));
}

function startMultiPlayer() {
	startGame(() => new RemoteServer(mpAddress.value, mpName.value));
}

function checkSupport() {
	var e;
	if (!window.JSON) {
		throw "Natiivi JSON-tuki puuttuu!";
	}

	if (!canvas.getContext) {
		throw "Canvas-tuki puuttuu!";
	}

	try {
		if (!window.localStorage)
		window.localStorage = {
			length: 0,
			data: {},
			getItem: function(key) {
				return this.data[key];
			},
			setItem: function(key, val) {
				if (typeof this.data[key] == "undefined") {
					++this.length;
				}
				this.data[key] = val;
			},
			removeItem: function(key) {
				this.setItem(key, null);
				--this.length;
				delete this.data[key];
			},
			clear: function() {
				this.data = {};
			},
			key: function(i) {
				for (var key in this.data) {
					if (--i == 0) return key;
				}
				return null;
			}
		};
	} catch (e) {
	}
}

window.onload = function() {
	var e;
	canvas = document.getElementById('canvas');
	mapCanvas = document.getElementById('mapCanvas');

	shadowBox = document.getElementById("shadow");
	textureBox = document.getElementById("texture");
	mapBox = document.getElementById("map");
	delayBox = document.getElementById('delay');

	fpsLabel = document.getElementById('fps').firstChild;

	spStart = document.getElementById("spStart");
	spLevel = document.getElementById("spLevel");
	mpName = document.getElementById("mpName") || {"value": "Nimimerkki"};
	mpAddress = document.getElementById("mpAddress");
	mpConnect = document.getElementById("mpConnect");
	mpDisconnect = document.getElementById("mpDisconnect");

	try {
		checkSupport();
	} catch (e) {
		alert(e);
		return;
	}

	restoreName();

	spStart.onclick = startSinglePlayer;
	spLevel.onfocus = function() { spLevelEditing = true; };
	spLevel.onblur = function() { spLevelEditing = false; };
	canvas.onkeydown = e => e.preventDefault();

	var mp = document.getElementById("mp");
	if (RemoteServer.supported && mp) {
		mpConnect.onclick = startMultiPlayer;
		mpDisconnect.onclick = endGame;
	} else if (mp) {
		mp.parentNode.removeChild(mp);
	}

	delayBox.onchange = delayBox.onkeyup = function() {
		var e;
		try {
			fps.delay = parseInt(this.value);
		} catch (e) {
		}
	};
	delayBox.onchange();

	levelImages.findImages();
	playerImages.findImages();
	ballImages.findImages();
	explosionImages.findImages();

	endGame();
	startSinglePlayer();
}

window.onunload = endGame;
