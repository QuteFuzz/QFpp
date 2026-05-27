import pennylane as qml

from .lib import Base


class pennylaneTesting(Base):
    def __init__(self, circuit, circuit_id: int):
        super().__init__(circuit, "pennylane", circuit_id)

    def _get_counts(self, opt_level: int):
        circuit = qml.set_shots(self.circuit, self.num_shots)

        if opt_level == 0:
            result = circuit()
        elif opt_level == 1:
            result = qml.transforms.cancel_inverses(circuit)()  # type:ignore
        elif opt_level == 2:
            result = qml.transforms.merge_rotations(qml.transforms.cancel_inverses(circuit))()  # type:ignore
        elif opt_level == 3:
            result = qml.compile(circuit)()  # type:ignore

        counts = self._preprocess_counts(dict(result))
        return counts
