var timer = {
	startTime: 0,
	realTime: 0,
	gameTime: 0,
	_readClock: function() {
		return (new Date()).getTime() / 1000;
	},
	_getTime: function() {
		return this._readClock() - this.startTime;
	},
	reset: function() {
		this.startTime = this._readClock();
		this.realTime = this.gameTime = 0;
	},
	nextFrame: function(dt) {
		this.realTime = this._getTime();
		if (this.realTime >= this.gameTime + dt) {
			this.gameTime += dt;
			return true;
		}
		return false;
	}
};
