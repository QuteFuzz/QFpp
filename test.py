from pytket import Circuit
from pytket.passes import GreedyPauliSimp

c = Circuit(2)
GreedyPauliSimp().apply(c)
