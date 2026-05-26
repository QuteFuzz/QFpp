import numpy as np
from qiskit import QuantumCircuit, transpile
from qiskit.transpiler import CouplingMap
from qiskit_aer import AerSimulator

from .lib import Base


class qiskitTesting(Base):
    def __init__(self, main_circ_bindings: dict) -> None:
        super().__init__("qiskit", endianess="big")
        self.bindings = main_circ_bindings

    def _make_line_topology(self, n_qubits: int):
        return CouplingMap.from_line(n_qubits)

    def _make_star_topology(self, n_qubits: int):
        return CouplingMap.from_ring(n_qubits)

    def _make_hex_topology(self, n_qubits: int):
        if n_qubits <= 17:
            return CouplingMap.from_heavy_hex(3)
        elif n_qubits <= 57:
            return CouplingMap.from_heavy_hex(5)
        else:
            return CouplingMap.from_heavy_hex(7)

    def _get_statevector(self, circuit, opt_level) -> np.ndarray:
        backend = AerSimulator(method="statevector")
        circ_prime = transpile(circuit, backend, optimization_level=opt_level)
        circ_prime.save_statevector()
        result = backend.run(circ_prime).result()

        sv = result.get_statevector()
        return np.asarray(sv)

    def _get_counts(self, circuit: QuantumCircuit, opt_level: int, circuit_num: int):
        backend = AerSimulator()

        cmap = self.topo_maker(circuit.num_qubits) if circuit.num_qubits >= 2 else None

        if cmap is not None and opt_level >= 1:
            basis_gates = ["cx", "id", "rz", "sx", "x", "y", "z", "h", "s", "t"]
            circ_prime = transpile(
                circuit, basis_gates=basis_gates, coupling_map=cmap, optimization_level=opt_level
            )
        else:
            circ_prime = transpile(circuit, backend=backend, optimization_level=opt_level)

        circ_bound = circ_prime.assign_parameters(self.bindings)

        counts = self._preprocess_counts(
            backend.run(circ_bound, shots=self.num_shots).result().get_counts()
        )

        if self.plot:
            self._plot_histogram(
                res=counts,
                title=f"qiskit_opt{opt_level}",
                circuit_number=circuit_num,
            )

        return counts
