function LocalServer(name) {
	level.parse(spLevel.value);
	this.name = name;
	var start = level.starts[0];
	var self = {
		"pos": {
			"x": start.x,
			"y": start.y
		},
		"dir": 0,
		"keys": {
			"up": false,
			"down": false,
			"left": false,
			"right": false,
			"shoot": false
		},
		"name": name,
		"score": 0,
		"hp": 1
	};
	var enemy = JSON.parse(JSON.stringify(self));
	if (self.name == "Dummy") {
		enemy.name = "Dummie";
	} else {
		enemy.name = "Dummy";
	}
	if (level.starts.length > 1) {
		enemy.pos.x = level.starts[1].x;
		enemy.pos.y = level.starts[1].y;
	}
	this.shoot_time = 0;
	players.set([self, enemy], self.name);
	timer.reset();
}

LocalServer.prototype.disconnect = function() {
}

LocalServer.prototype.send = function(input) {
	input.copyTo(players.self.keys);
	if (input.shoot && this.shoot_time + 1 < timer.gameTime) {
		this.shoot_time = timer.gameTime;
		balls.add(this.shoot_time, {
			"pos": {
				"x": players.self.pos.x,
				"y": players.self.pos.y
			},
			"speed": {
				"x": Math.cos(players.self.dir) * 4,
				"y": Math.sin(players.self.dir) * 4
			}
		});
	}
}

