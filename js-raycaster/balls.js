var balls = {
	data: {},
	add: function(id, ball) {
		this.data[id] = ball;
		ball.time = 0;
	},
	remove: function(id) {
		if (this.data[id]) {
			delete this.data[id];
		}
	},
	move: function(dt) {
		while (dt > 0.1) {
			this.move(0.1);
			dt -= 0.1;
		}
		var rm = {};
		for (var id in this.data) {
			if (!this._move(this.data[id], dt)) {
				rm[id] = 1;
			}
		}
		for (var id in rm) {
			this.remove(id);
		}
	},
	_move: function(ball, dt) {
		ball.time += dt;
		ball.pos.x += ball.speed.x * dt;
		ball.pos.y += ball.speed.y * dt;
		return !level.get(ball.pos.x, ball.pos.y);
	}
};
