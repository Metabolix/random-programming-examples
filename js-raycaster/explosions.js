var explosions = [];

explosions.duration = 0.3;

explosions.add = function(pos) {
	this.push({
		"pos": pos,
		"time": 0
	});
};

explosions.move = function(dt) {
	for (var i = 0; i < this.length; ++i) {
		this[i].time += dt;
	}
	while (this.length && this[0].time > this.duration) {
		this.shift();
	}
};
