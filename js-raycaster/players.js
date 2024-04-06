var players = {
	data: {},
	self: null,
	set: function(infos, self) {
		var exists = {};
		for (var i = 0; i < infos.length; ++i) {
			var info = infos[i];
			var name = info.name;
			info.pos2 = info.pos;
			if (!this.data[name] || !this.data[name].hp) {
				info.animation = 0;
			} else {
				info.animation = this.data[name].animation;
				info.pos = this.data[name].pos;
			}
			this.data[name] = info;
			exists[name] = true;
		}
		for (var name in this.data) {
			if (!exists[name]) {
				delete this.data[name];
			}
		}
		this.self = this.data[self];
	},
	each: function(func) {
		for (var name in this.data) {
			func(this.data[name]);
		}
	},
	move: function(dt) {
		for (var name in this.data) {
			this._move(this.data[name], dt);
		}
	},
	_move: function(p, dt) {
		var maxSpeed = 2.5;
		var speed = ((p.keys.up ? 1 : 0) - (p.keys.down ? 1 : 0)) * maxSpeed;
		var turn = ((p.keys.left ? 1 : 0) - (p.keys.right ? 1 : 0)) * Math.PI * 1.5;
		var dx = speed * dt * Math.cos(p.dir);
		var dy = speed * dt * Math.sin(p.dir);
		if (speed) {
			p.animation += 1.5 * dt;
		} else {
			p.animation = 0;
		}

		this._move2(p.pos2, p.pos2.x + dx, p.pos2.y + dy);
		if (p.pos != p.pos2) {
			this._move2(p.pos, p.pos.x + dx, p.pos.y + dy);
			var speed = 2 * maxSpeed * dt;
			var dx = p.pos2.x - p.pos.x;
			var dy = p.pos2.y - p.pos.y;
			p.pos.x += (Math.abs(dx) > speed) ? dx / Math.abs(dx) * speed : dx;
			p.pos.y += (Math.abs(dy) > speed) ? dy / Math.abs(dy) * speed : dy;
		}
		p.dir += turn * dt;
	},
	_move2: function(pos, tmp_x, tmp_y) {
		if (!level.get(tmp_x, tmp_y) && !level.get(tmp_x, pos.y) && !level.get(pos.x, tmp_y)) {
			pos.x = tmp_x;
			pos.y = tmp_y;
		} else if (!level.get(tmp_x, pos.y)) {
			pos.x = tmp_x;
		} else if (!level.get(pos.x, tmp_y)) {
			pos.y = tmp_y;
		}
	}
};
