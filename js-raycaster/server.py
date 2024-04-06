#!/usr/bin/python3 -OO
# -*- coding: UTF-8 -*-

import asyncio
from aiohttp import web
import os
import time
import random
import math

version = "2024-04-06"
phys_frame_len = 0.03

class Point:
    def __init__(self, x = 0.0, y = 0.0):
        self.x = float(x)
        self.y = float(y)

    def __add__(self, other):
        return Point(self.x + other.x, self.y + other.y)

    def __str__(self):
        data = (self.x, self.y)
        data = "%.3f:%.3f" % data
        return data

    def floor(self, precision):
        if self.x < 0:
            self.x = math.floor(self.x / precision) * precision
        else:
            self.x = math.ceil(self.x / precision) * precision
        if self.y < 0:
            self.y = math.floor(self.y / precision) * precision
        else:
            self.y = math.ceil(self.y / precision) * precision

class Input:
    def __init__(self, data = []):
        if type(data) != list or len(data) != 5:
            data = [0,0,0,0,0]
        self.up, self.down, self.left, self.right, self.shoot = data

class Ball:
    nextid = 0
    def __init__(self, player):
        speed = 4.0
        self.hitrange = 0.8
        self.player = player
        self.speed = Point(speed * math.cos(player.direction), speed * math.sin(player.direction))
        self.pos = Point(player.pos.x, player.pos.y)

    def hits(self, player):
        dx = self.pos.x - player.pos.x
        dy = self.pos.y - player.pos.y
        return dx * dx + dy * dy < self.hitrange * self.hitrange

    def __str__(self):
        data = (self.pos.x, self.pos.y, self.speed.x, self.speed.y)
        data = "%.3f:%.3f:%.3f:%.3f" % data
        return data

class Player:
    def __init__(self):
        self.pos = Point(0.0, 0.0)
        self.direction = 0
        self.keys = Input()
        self.name = None
        self.score = 0
        self.hp = 0
        self.deathcooldown = 1.0
        self.shootcooldown = 0
        self.playersStr = None
        self.queue = asyncio.Queue()

    async def connect(self, socket):
        self.socket = socket
        if await self.recv() != version:
            raise Exception("Version mismatch!")

    def __str__(self):
        data = (
            self.pos.x, self.pos.y, self.direction,
            self.keys.up, self.keys.down, self.keys.left, self.keys.right, self.keys.shoot,
            self.name, self.score, self.hp
        )
        data = "%.3f:%.3f:%.3f:%d:%d:%d:%d:%d:%s:%d:%d" % data
        return data

    async def send(self, data):
        try:
            return await self.socket.send_str(data)
        except:
            await self.socket.close()

    async def recv(self):
        try:
            return await self.socket.receive_str()
        except:
            await self.socket.close()

    async def sendPlayers(self, data):
        if data != self.playersStr:
            self.playersStr = data
            await self.send(data)

    def insertQueue(self, data):
        self.queue.put_nowait(data)

    async def sendQueued(self):
        while not self.queue.empty():
            data = self.queue.get_nowait()
            if type(data) == tuple:
                data = "".join(str(x) for x in data)
            await self.send(data)

    def updateKeys(self, data):
        if data == None or len(data) != 9:
            raise Exception("Invalid message!")
        data = data.split(":")
        if len(data) != 5:
            raise Exception("Invalid message!")
        data = [int(x) for x in data]
        return Input(data)

    def _checkName(self, name):
        if len(name) < 2:
            return False
        for char in name:
            if char.isalnum(): continue
            if char in "-_.åäöÅÄÖ": continue
            return False
        return True

    async def askName(self):
        while self.name == None:
            await self.send("NAME")
            while True:
                tmp = await self.recv()
                if tmp.startswith("NAME:"):
                    tmp = tmp[5:]
                    break
            if self._checkName(tmp):
                self.name = tmp

class Level:
    def __init__(self, data):
        self.data = data
        self.rows = [x.strip() for x in data.strip().split("\n")]
        if len(self.rows) < 4 or min(len(x) for x in self.rows) < 16:
            raise Exception("Level is too small!")
        self.rows.reverse()
        self.startpoints = []
        for y in range(0, len(self.rows)):
            for x in range(0, len(self.rows[y])):
                if self.rows[y][x] == "X":
                    self.startpoints.append(Point(x + 0.5, y + 0.5))
            self.rows[y] = self.rows[y].replace("X", " ")

        if len(self.startpoints) == 0:
            self.startpoints.append(Point(len(self.rows[0]) / 2, len(self.rows) / 2))

    def wall(self, x, y):
        if 0 <= y and y < len(self.rows):
            y = int(y)
            if 0 <= x and x < len(self.rows[y]):
                x = int(x)
                if self.rows[y][x] == " ":
                    return False
        return True

class Game:
    def __init__(self, level):
        self.level = level
        self.players = []
        self.balls = []
        self.time = 0
        self.playersStr = None
        self.running = True
        self.reset()

    def reset(self):
        self.balls = []
        self.time = time.time()

    async def addPlayer(self, player):
        while player.name == None:
            await player.askName()
            for other in self.players:
                if player.name == other.name:
                    player.name = None
        await player.send("LEVEL;" + self.level.data)
        if len(self.players) == 0:
            self.reset()
        self.players.append(player)
        self.playersStr = None
        self.respawn(player)
        await self.sendPlayers(player)
        await self.sendBalls(player)

    def removePlayer(self, player):
        self.players.remove(player)

    async def sendPlayers(self, player):
        if self.playersStr == None:
            data = ["PLAYERS"]
            for other in self.players:
                data.append(str(other))
            self.playersStr = ";".join(data)
        await player.sendPlayers(self.playersStr)

    def addBall(self, ball):
        try:
            i = self.balls.index(None)
        except:
            i = len(self.balls)
            self.balls.append(None)
        self.balls[i] = ball
        for player in self.players:
            # Use a reference (tuple) so the player gets the most
            # recent position when the message is actually sent.
            player.insertQueue(("BALL;" + str(i) + ":", ball))

    def explodeBall(self, ball):
        i = self.balls.index(ball)
        self.balls[i] = None
        data = "EXPLODE;" + str(i) + ":" + str(ball.pos)
        for player in self.players:
            player.insertQueue(data)

    async def sendBalls(self, player):
        for i in range(0, len(self.balls)):
            ball = self.balls[i]
            if ball == None: continue
            data = "BALL;" + str(i) + ":" + str(ball)
            await player.send(data)

    async def communicate(self, player):
        msg = await player.recv()
        if not msg:
            return False
        keys = player.updateKeys(msg)
        self.update()
        player.keys = keys;
        await self.sendPlayers(player)
        await player.sendQueued()
        return True

    def respawn(self, player):
        r = random.randint(0, len(self.level.startpoints) - 1)
        player.pos = self.level.startpoints[r]
        player.hp = 10
        player.deathcooldown = 0.0

    def wall(self, x, y):
        return self.level.wall(x, y)

    def update(self):
        dt = phys_frame_len
        while self.time < time.time():
            self.playersStr = None
            self.move(dt)
            self.time += dt

    def move(self, dt):
        for player in self.players:
            if player.deathcooldown <= 0:
                self.move1(player, dt)
            else:
                player.deathcooldown -= dt
                if player.deathcooldown <= 0:
                    self.respawn(player)
        for i in range(0, len(self.balls)):
            ball = self.balls[i]
            if ball != None:
                self.move3(ball, dt)

    def move1(self, p, dt):
        speed = (p.keys.up - p.keys.down) * 2.5
        turn = (p.keys.left - p.keys.right) * math.pi * 1.5
        change = Point(speed * dt * math.cos(p.direction), speed * dt * math.sin(p.direction))
        change.floor(0.001)
        next = p.pos + change
        next2 = next + change
        p.pos = self.move2(p.pos, next2, p.pos, next)
        p.direction += turn * dt
        if p.shootcooldown > 0:
            p.shootcooldown -= dt
        elif p.keys.shoot != 0:
            p.shootcooldown = 0.5
            self.addBall(Ball(p))

    def move2(self, p1, p2, ret1, ret2):
        if not (self.wall(p2.x, p2.y) or self.wall(p2.x, p1.y) or self.wall(p1.x, p2.y)):
            return ret2;
        elif not self.wall(p2.x, p1.y):
            return Point(ret2.x, ret1.y)
        elif not self.wall(p1.x, p2.y):
            return Point(ret1.x, ret2.y)
        return ret1

    def move3(self, ball, dt):
        change = Point(dt * ball.speed.x, dt * ball.speed.y)
        change.floor(0.001)
        next = ball.pos + change
        next2 = next + change
        ball.pos = self.move2(ball.pos, next2, ball.pos, next)
        explode = False
        for player in self.players:
            if player != ball.player and ball.hits(player):
                explode = True
                if player.hp > 0:
                    player.hp -= 1
                    if player.hp == 0:
                        ball.player.score += 1
                        player.deathcooldown = 3.0
                break
        if explode or ball.pos != next:
            self.explodeBall(ball)

    def end(self):
        self.running = False

async def websocket_handler(request, game):
    ws = web.WebSocketResponse()
    await ws.prepare(request)

    player = Player()
    try:
        await player.connect(ws)
        await game.addPlayer(player)
    except Exception as e:
        print("Player(ws): %s" % (e,))
        return ws

    print("+ %s.%03d, %s, %s" % (time.strftime("%Y-%m-%d %H:%M:%S"), int(1000 * (time.time() % 1)), request.remote, player.name))
    try:
        while game.running:
            if not await game.communicate(player):
                break
    except Exception as e:
        print("game.communicate: %s" % (e,))
    finally:
        await player.send("QUIT")
        game.removePlayer(player)
        print("- %s.%03d, %s, %s" % (time.strftime("%Y-%m-%d %H:%M:%S"), int(1000 * (time.time() % 1)), request.remote, player.name))
    return ws

def main():
    level = Level(open("level.txt", encoding="ascii").read())
    game = Game(level)
    print("Loaded level, starting server...")
    app = web.Application()
    def responder(file):
        return lambda r: web.FileResponse(file)
    for file in os.listdir("."):
        if any(file.endswith(ext) for ext in [".html", ".js", ".css", ".png", ".jpg", "server.py", "level.txt"]):
            app.router.add_route('GET', '/' + file, responder(file))
    app.router.add_route('GET', '/', responder('index.html'))
    app.router.add_route('GET', '/server.websocket', lambda r: websocket_handler(r, game))
    web.run_app(app, host = 'localhost', port = 65432, reuse_address = True)
    game.end()

if __name__ == "__main__":
    main()
