import javax.sound.sampled.*;
import java.util.*;
import javax.swing.*;
import javax.swing.table.*;
import javax.swing.event.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.plaf.metal.MetalLookAndFeel;

public class AbsolutePitch extends JApplet {
	public static void main(String[] args) {
		SwingUtilities.invokeLater(new Runnable() {
			public void run() {
				JFrame frame = new JFrame("Absolute (?) pitch training");

				UIManager.put("swing.boldMetal", Boolean.FALSE);
				try {
					UIManager.setLookAndFeel(new MetalLookAndFeel());
				} catch (UnsupportedLookAndFeelException e) {
				}
				SwingUtilities.updateComponentTreeUI(frame);

				frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
				AbsolutePitchPanel app = new AbsolutePitchPanel();
				frame.add(app);
				frame.pack();
				frame.setVisible(true);
				app.start();
			}
		});
	}

	private AbsolutePitchPanel app;

	@Override
	public void init() {
		app = new AbsolutePitchPanel();
		getContentPane().add(app, BorderLayout.CENTER);
	}

	@Override
	public void start() {
		app.start();
	}

	@Override
	public void stop() {
		app.stop();
	}

	@Override
	public void destroy() {
		app.destroy();
	}

	public static class AbsolutePitchPanel extends JPanel {
		private SourceDataLine audioLine;
		private Random random = new Random();
		private JLabel answerLabel;

		private String[] pitchName = {
			"a", "a♯",  "c♭",  "c",   "c♯",  "d", "e♭",  "e",   "f",   "f♯",  "g", "a♭"
			// "a", "b",   "h",   "c",   "cis", "d", "es",  "e",   "f",   "fis", "g", "as"
			// "a", "ais", "ces", "his", "des", "d", "dis", "fes", "eis", "ges", "g", "gis"
		};
		private double frequencyPitch0 = 440;
		private int pitchMin = -16, pitchMax = 8;

		private int currentPitch;
		private boolean canAnswer = false;

		private int[][] count = new int[12][12];
		private MyTableModel tableModel;
		private JTable table;

		private double angle = 0;

		private static double RA(double f) {
			return 12200 * 12200 * f * f * f * f / (f * f + 20.6 * 20.6) / (f * f + 12200 * 12200) / Math.sqrt((f * f + 107.7 * 107.7) * (f * f + 737.9 * 737.9));
		}

		private byte sample(double dt, double p, double vol) {
			double frequency = frequencyPitch0 * Math.pow(2, (p / 12));
			angle += 2 * Math.PI * dt * frequency;
			vol *= Math.min(0.2 * 1.75 / (RA(frequency) + RA(frequency * 2) * 0.5 + RA(frequency * 4) * 0.25), 1);
			double s = Math.sin(angle); // + 0.1 * Math.sin(angle * 2) + 0.03 * Math.sin(angle * 4);
			s = (s < 0 ? -1 : 1) * Math.pow(s < 0 ? -s : s, 0.8);
			return (byte)(int)(127 * s * vol - random.nextInt(2));
		}

		private int randomPitch() {
			return pitchMin + random.nextInt(pitchMax - pitchMin);
		}

		private void playNext() {
			if (audioLine == null) {
				return;
			}
			new PlayerThread().start();
		}

		public void start() {
			if (audioLine != null) {
				audioLine.start();
				playNext();
			}
		}

		public void stop() {
			if (audioLine != null) {
				// audioLine.drain();
				audioLine.stop();
			}
		}

		public void destroy() {
			if (audioLine != null) {
				audioLine.close();
				audioLine = null;
			}
		}

		private class PlayerThread extends Thread {
			@Override
			public void run() {
				synchronized(audioLine) {
					int p0 = randomPitch();
					int rate = (int)audioLine.getFormat().getSampleRate();
					int n = 0;
					int distractionLength = rate * 9 / 2;
					byte[] data = new byte[distractionLength + 30 * (rate / 10)];
					for (int i = 0, len = rate / 10; i < len; ++i) {
						data[n++] = (sample(1.0 / rate, p0, (double)i / len));
					}
					for (int len, lenMax = distractionLength; (len = Math.min(lenMax, rate / (2 + random.nextInt(9)))) > 0; lenMax -= len) {
						if (len + rate / 10 > lenMax) {
							len = lenMax;
						}
						int p1 = randomPitch();
						for (int i = 0; i < len; ++i) {
							data[n++] = (sample(1.0 / rate, p0 + (p1 - p0) * (double)i / len, 1));
						}
						p0 = p1;
					}
					int m = n;
					for (int i = 0, len = 28 * (rate / 10); i < len; ++i) {
						data[n++] = (sample(1.0 / rate, p0, 1));
					}
					for (int i = 0, len = rate / 10; i < len; ++i) {
						data[n++] = (sample(1.0 / rate, p0, (1 - (double)i / len)));
					}
					currentPitch = ((p0 % 12) + 12) % 12;

					audioLine.start();
					audioLine.write(data, 0, m);
					audioLine.write(data, m, rate);
					answerLabel.setText("Click on the right note (table column)!");
					canAnswer = true;
					m += rate;
					audioLine.write(data, m, n - m);
				}
			}
		}

		private void answer(int pitch) {
			if (!canAnswer) {
				return;
			}
			pitch = ((pitch % 12) + 12) % 12;
			int error = ((pitch - currentPitch) % 12 + 12 + 6) % 12 - 6;
			count[currentPitch][(error + 12) % 12] += 1;
			int countRight = 0, countAll = 0;
			for (int[] t: count) {
				countRight += t[0];
				for (int i: t) {
					countAll += i;
				}
			}
			answerLabel.setText(
				"Accuracy: " + countRight + "/" + countAll + " = " + (int)(100.0 * countRight / countAll + 0.5) + "%. " +
				"Last: " + pitchName[currentPitch] + ". " +
				(
					error != 0
					? "Your guess (" + pitchName[pitch] + ") was " + error + " semitones too " + (error > 0 ? "high" : "low") + "."
					: "+1 point!"
				)
			);
			tableModel.change(currentPitch, error);
			canAnswer = false;
			playNext();
		}

		private class MyTableModel extends AbstractTableModel {
			@Override
			public int getColumnCount() {
				return 15;
			}

			@Override
			public int getRowCount() {
				return 13;
			}

			@Override
			public Class getColumnClass(int i) {
				return Integer.class;
			}

			@Override
			public String getColumnName(int i) {
				switch (i) {
					case 0: return "error";
					case 1: return "total";
					case 2: return "";
				}
				return pitchName[i - 3];
			}

			@Override
			public Object getValueAt(int row, int col) {
				int pitch = col - 3;
				int error = 5 - row;
				if (row == 12) {
					switch (col) {
						case 0: return "acc.";
						case 1: {
							int s = 0, all = 0;
							for (int[] t: count) {
								s += t[0];
								for (int i: t) {
									all += i;
								}
							}
							return (int)(100.0 * s / all + 0.5) + "%";
						}
						case 2: return "";
					}
					int s = 0;
					for (int i: count[pitch]) {
						s += i;
					}
					return (int)(100.0 * count[pitch][0] / s + 0.5) + "%";
				}
				switch (col) {
					case 0: return error > 0 ? ("+" + error) : ("" + error);
					case 1: {
						int s = 0;
						for (int[] t: count) {
							s += t[(error + 12) % 12];
						}
						return s;
					}
					case 2: return "";
				}
				return count[pitch][(error + 12) % 12];
			}

			public void change(int pitch, int error) {
				int row = 5 - error; // 5 - row == error
				int totalCol = 1;
				int pitchCol = pitch + 3;
				table.clearSelection();
				table.changeSelection(row, pitchCol, true, false);
				table.tableChanged(new TableModelEvent(this, row, row, totalCol, TableModelEvent.UPDATE));
				table.tableChanged(new TableModelEvent(this, row, row, pitchCol, TableModelEvent.UPDATE));
				row = 12;
				table.tableChanged(new TableModelEvent(this, row, row, totalCol, TableModelEvent.UPDATE));
				table.tableChanged(new TableModelEvent(this, row, row, pitchCol, TableModelEvent.UPDATE));
			}
		}

		public AbsolutePitchPanel() {
			setPreferredSize(new Dimension(640, 480));
			int sampleRate = 48000;
			AudioFormat format = new AudioFormat(sampleRate, 8, 1, true, false);
			try {
				audioLine = AudioSystem.getSourceDataLine(format);
				audioLine.open(format);
			} catch (LineUnavailableException e) {
				System.out.println(e);
				audioLine = null;
				this.add(new JLabel("Audio not available (" + sampleRate + " Hz 8-bit signed mono)"), BorderLayout.CENTER);
				return;
			}

			tableModel = new MyTableModel();
			table = new JTable(tableModel);

			MouseListener mouseListener = new MouseAdapter() {
				@Override
				public void mouseClicked(MouseEvent e) {
					int i = table.columnAtPoint(e.getPoint());
					if (3 <= i && i < 15) {
						answer(i - 3);
					}
				}
			};

			table.addMouseListener(mouseListener);
			table.getTableHeader().addMouseListener(mouseListener);
			table.getTableHeader().setResizingAllowed(false);
			table.getTableHeader().setReorderingAllowed(false);

			this.add(answerLabel = new JLabel("Listen to the sound and try to guess the pitch!"), BorderLayout.PAGE_START);
			this.add(new JScrollPane(table), BorderLayout.CENTER);
		}
	}
}
