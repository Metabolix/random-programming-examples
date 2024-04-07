#!/usr/bin/python3

import math
import random

notes = [
	"a", "b",   "h",   "c",   "cis", "d",  "es",   "e",   "f", "fis", "g", "as",
	"a", "ais", "ces", "his", "des", "d", "dis", "fes", "eis", "ges", "g", "gis",
	"a", "b",   "h",   "c",   "c#",  "d",  "eb",   "e",   "f", "f#",  "g", "ab",
	"a", "a#",  "cb",  "h#",  "db",  "d",  "d#",  "fb",  "e#", "gb",  "g", "g#"
]

import pyaudio

class AudioDevice:
	def __init__(self):
		self.pyaudio = pyaudio.PyAudio()
		self.rate = 48000
		self.stream = self.pyaudio.open(format=pyaudio.paInt8, channels=1, rate=self.rate, output=True)
	def play(self, data):
		self.stream.write(bytes(data))
	def close(self):
		self.stream.stop_stream()
		self.stream.close()
		self.pyaudio.terminate()

def generate_samples(p_min, p_max, rate):
	angle = 0
	def floatsample(dt, p):
		nonlocal angle
		frequency = 440 * (2 ** (p / 12))
		angle += 2 * math.pi * dt * frequency
		return math.sin(angle) + 0.03 * math.sin(angle * 2) + 0.01 * math.sin(angle * 4)

	def bytesample(f):
		return int(127 * f) & 0xff

	p0 = random.randint(p_min, p_max)
	data = bytearray()
	for i in range(0, rate // 10):
		data.append(bytesample(floatsample(1 / rate, p0) * i / (rate // 10)))
	for i in range(0, 20):
		p1 = random.randint(p_min, p_max)
		change_length = rate // random.randint(2, 10)
		for i in range(0, change_length):
			data.append(bytesample(floatsample(1 / rate, p0 + (p1 - p0) * i / change_length)))
		p0 = p1
	for i in range(0, rate * 2):
		data.append(bytesample(floatsample(1 / rate, p0)))
	for i in range(0, rate // 10):
		data.append(bytesample(floatsample(1 / rate, p0) * (1 - i / (rate // 10))))
	for i in range(0, 8 * rate // 10):
		data.append(bytesample(0))

	return p0, data

def stats_str(answers):
	s = [[]]
	s[-1].append("%4s" % "")
	for i in range(0, 12):
		s[-1].append("%4s" % notes[i])
	for j in range(5, -7, -1):
		s.append([])
		s[-1].append("%+4d" % j)
		for i in range(0, 12):
			if answers[i][j] > 0:
				s[-1].append("%4d" % answers[i][j])
			else:
				s[-1].append("%4s" % ".")
	s.append(s[0])
	return "\n".join("".join(row) for row in s)

def main():
	audio_device = AudioDevice()
	answers = dict((i, dict((j, 0) for j in range(-6, 6))) for i in range(0, 12))

	p_min = -16
	p_max = p_min - 1 + 12 * 2
	n = n_ok = 0
	try:
		while True:
			print(stats_str(answers))
			correct, samples = generate_samples(p_min, p_max, audio_device.rate)
			print("Listen closely, then enter the note name, or 'exit' to quit.")
			audio_device.play(samples)
			try:
				answer_str = input("? = ").lower()
				if answer_str in ("exit", "quit", "q"):
					break
				answer = notes.index(answer_str)
				offset = ((answer - correct + 6) % 12) - 6
				answers[correct % 12][offset] += 1
				if offset == 0:
					print("OK")
					n_ok += 1
				else:
					print("%+d off, correct = %s" % (offset, notes[correct % 12]))
			except ValueError:
				print("??, %s" % (notes[correct % 12]))
			n += 1
	except KeyboardInterrupt:
		pass
	except EOFError:
		pass
	finally:
		audio_device.close()

	print(stats_str(answers))
	if n > 0:
		print("n = %d, ok = %d, ratio = %.3f" % (n, n_ok, n_ok / n))
	else:
		print("n = 0")

if __name__ == "__main__":
	main()
