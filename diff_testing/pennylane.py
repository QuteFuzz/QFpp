import pennylane as qml

from .lib import Base


class pennylaneTesting(Base):
    def __init__(self):
        super().__init__("pennylane")

    def get_counts(self, circuit, opt_level: int, circuit_num: int):
        circuit = qml.set_shots(circuit, self.num_shots)

        if opt_level == 0:
            result = circuit()
        elif opt_level == 1:
            result = qml.transforms.cancel_inverses(circuit)()
        elif opt_level == 2:
            result = qml.transforms.merge_rotations(qml.transforms.cancel_inverses(circuit))()
        elif opt_level == 3:
            result = qml.compile(circuit)()

        counts = self.preprocess_counts(dict(result), n_bits=len(list(result.keys())[0]))
        return counts
