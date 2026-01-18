# import datetime
# import sys
# from typing import Any, Dict, List

import traceback

from pytket._tket.circuit import Circuit
from pytket.extensions.qiskit.backends.aer import AerBackend, AerStateBackend

from .lib import Base

# from pytket.circuit import OpType
# from pytket.extensions.qiskit.backends.aer import AerBackend, AerStateBackend
# from pytket.passes import AutoRebase, DecomposeBoxes, FlattenRelabelRegistersPass
# from pytket.qir.conversion.api import QIRFormat, pytket_to_qir


class pytketTesting(Base):
    def __init__(self) -> None:
        super().__init__("pytket")

    def ks_diff_test(self, circuit: Circuit, circuit_number: int) -> None:
        """
        Runs circuit on pytket simulator and returns counts
        """
        backend = AerBackend()

        # Get original circuit shots
        uncompiled_circ = backend.get_compiled_circuit(circuit, optimisation_level=0)
        handle1 = backend.process_circuit(uncompiled_circ, n_shots=100000)
        result1 = backend.get_result(handle1)
        counts1 = self.preprocess_counts(result1.get_counts())

        # Compile circuit at 3 different optimisation levels
        for i in range(3):
            compiled_circ = backend.get_compiled_circuit(circuit, optimisation_level=i + 1)

            # Process the compiled circuit
            handle2 = backend.process_circuit(compiled_circ, n_shots=100000)
            result2 = backend.get_result(handle2)
            counts2 = self.preprocess_counts(result2.get_counts())

            # Run the kstest on the two results
            ks_value = self.ks_test(counts1, counts2, 100000)
            print(f"Optimisation level {i + 1} ks-test p-value: {ks_value}")

            # plot results
            if self.plot:
                self.plot_histogram(counts1, "Uncompiled Circuit Results", 0, circuit_number)
                self.plot_histogram(counts2, "Compiled Circuit Results", i + 1, circuit_number)

    def run_circ_statevector(self, circuit: Circuit, circuit_number: int) -> None:
        """
        Runs circuit on pytket simulator and returns statevector
        """
        try:
            backend = AerStateBackend()
            # circuit with no passes
            uncompiled_circ = backend.get_compiled_circuit(circuit.copy(), optimisation_level=0)
            no_pass_statevector = uncompiled_circ.get_statevector()

            # Get statevector after every optimisation level
            for i in range(3):
                compiled_circ = backend.get_compiled_circuit(
                    circuit.copy(), optimisation_level=i + 1
                )
                pass_statevector = compiled_circ.get_statevector()
                dot_prod = self.compare_statevectors(no_pass_statevector, pass_statevector, 6)

                if dot_prod == 1:
                    print("Statevectors are the same\n")
                else:
                    print("Statevectors not the same")
                    print("Dot product: ", dot_prod)

        except Exception:
            print("Exception :", traceback.format_exc())

    # def run_guppy_pytket_diff(
    #     self,
    #     circuit: Circuit,
    #     circuit_number: int,
    #     qubit_defs_list: List[int],
    #     bit_defs_list: List[int],
    # ) -> None:
    #     """
    #     Loads pytket circuit as a guppy circuit and runs it, comparing the results
    #     with normal AerBackend
    #     """
    #     pytket_circ_copy = circuit.copy()

    #     # Define the guppy gateset
    #     guppy_gateset = {
    #         OpType.CX,
    #         OpType.CZ,
    #         OpType.CY,
    #         OpType.X,
    #         OpType.Y,
    #         OpType.Z,
    #         OpType.H,
    #         OpType.T,
    #         OpType.Tdg,
    #         OpType.Rx,
    #         OpType.Ry,
    #         OpType.Rz,
    #         OpType.S,
    #         OpType.Sdg,
    #         OpType.CCX,
    #         OpType.V,
    #         OpType.Vdg,
    #         OpType.CRz,
    #     }
    #     DecomposeBoxes().apply(circuit)  # Decompose circboxes for guppy compatibility
    #     AutoRebase(guppy_gateset).apply(circuit)  # Rebase to guppy gateset

    #     guppy_circuit = guppy.load_pytket("guppy_circuit", circuit)

    #     # Reordering qubits in alphabetical order like in pytket
    #     qubit_defs_list_sorted = [
    #         x[1] for x in sorted(enumerate(qubit_defs_list), key=lambda x: (x[1] == 0, x[0]))
    #     ]
    #     # Reordering bits in alphabetical order like in pytket
    #     bit_defs_list_sorted = [x for x in bit_defs_list if x == 0] + [
    #         x for x in bit_defs_list if x != 0
    #     ]
    #     # And if creg is the sole register, it will be expanded to individual bits
    #     if len(bit_defs_list_sorted) == 1 and bit_defs_list_sorted[0] != 0:
    #         bit_defs_list_sorted = [1] * bit_defs_list_sorted[0]

    #     @guppy.comptime
    #     def main() -> None:
    #         qubit_variables = []

    #         # qreg are always at the front, followed by singular qubits
    #         for qubit_def in qubit_defs_list_sorted:
    #             qubit_array = [qubit() for _ in range(qubit_def)] if qubit_def > 0 else [qubit()]
    #             qubit_variables.append(qubit_array)

    #         # At this point, bit and qubit ordering is different. Needs to be adapted here
    #         creg_results = guppy_circuit(*qubit_variables)

    #         for i in range(len(bit_defs_list_sorted)):
    #             if bit_defs_list_sorted[i] == 0:
    #                 result(f"b{i}", creg_results[i])

    #         for r in range(len(qubit_variables)):
    #             if isinstance(qubit_variables[r], list):
    #                 # measure_array returns a quantum result that guppy handles
    #                 result(f"q{r}", measure_array(qubit_variables[r]))

    #         if creg_results is not None and hasattr(creg_results, "__len__"):
    #             for i in range(len(bit_defs_list_sorted)):
    #                 if bit_defs_list_sorted[i] != 0:
    #                     result(f"creg{i}", creg_results[i])

    #     try:
    #         # Getting guppy circuit results
    #         compiled_circ = main.compile()
    #         runner = build(compiled_circ)
    #         results = QsysResult(
    #             runner.run_shots(Quest(), n_qubits=circuit.n_qubits, n_shots=10000)
    #         )
    #         counts_guppy = results.collated_counts()

    #         # Reconstruct dictionary from counts
    #         reconstructed_counts: Dict[Any, int] = {
    #             "".join([measurement[1] for measurement in key]): value
    #             for key, value in counts_guppy.items()
    #         }

    #         processed_counts_guppy = self.preprocess_counts(reconstructed_counts)
    #         print("Processed guppy counts:", processed_counts_guppy)

    #         # Getting uncompiled pytket circuit results
    #         backend = AerBackend()
    #         pytket_circ_copy.measure_all()
    #         uncompiled_pytket_circ = backend.get_compiled_circuit(
    #             pytket_circ_copy, optimisation_level=0
    #         )
    #         handle = backend.process_circuit(uncompiled_pytket_circ, n_shots=10000)
    #         result_pytket = backend.get_result(handle)
    #         counts_pytket = self.preprocess_counts(result_pytket.get_counts())
    #         print("Pytket counts:", counts_pytket)

    #         # Run the kstest on the two results
    #         ks_value = self.ks_test(processed_counts_guppy, counts_pytket, 10000)
    #         print(f"Guppy vs Pytket ks-test p-value: {ks_value}")

    #         # Heuristic to determine if the testcase is interesting
    #         if ks_value < 0.05:
    #             print(f"Interesting circuit found: {circuit_number}")
    #             self.save_interesting_circuit(
    #                 circuit_number, self.OUTPUT_DIR / "interesting_circuits"
    #             )

    #         if self.plot:
    #             self.plot_histogram(
    #                 processed_counts_guppy, "Guppy Circuit Results", 0, circuit_number
    #             )
    #             self.plot_histogram(counts_pytket, "Pytket Circuit Results", 0, circuit_number)

    #     except Exception as e:
    #         if isinstance(e, GuppyError):
    #             renderer = DiagnosticsRenderer(DEF_STORE.sources)
    #             renderer.render_diagnostic(e.error)
    #             sys.stderr.write("\n".join(renderer.buffer))
    #             sys.stderr.write("\n\nGuppy compilation failed due to 1 previous error\n")
    #         else:
    #             print("Error during compilation:", e)
    #             print("Exception :", traceback.format_exc())

    # def run_qir_pytket_diff(self, circuit: Circuit, circuit_number: int) -> None:
    #     """
    #     Loads pytket circuit as qir and runs it, comparing the results
    #     """
    #     if not self.qnexus_check_login_status():
    #         self.qnexus_login()

    #     try:
    #         # Convert pytket circuit to QIR
    #         qir_circuit = circuit.copy()
    #         # Flatten and relabel registers to ensure consistent naming
    #         FlattenRelabelRegistersPass().apply(qir_circuit)

    #         qir_LLVM = pytket_to_qir(qir_circuit, qir_format=QIRFormat.STRING)
    #         project = qnx.projects.get_or_create(name="qir_pytket_diff")
    #         qnx.context.set_active_project(project)
    #         qir_name = "pytket_qir_circuit" + str(circuit_number)
    #         jobname_suffix = datetime.datetime.now().strftime("%Y_%m_%d_%H_%M_%S")
    #         qir = pyqir.Module.from_ir(pyqir.Context(), qir_LLVM).bitcode
    #         qir_program_ref = qnx.qir.upload(qir=qir, name=qir_name, project=project)

    #         # Run the QIR on H1-Emulator emulator
    #         device_name = "H1-Emulator"

    #         qnx.context.set_active_project(project)
    #         config = qnx.QuantinuumConfig(device_name=device_name)

    #         job_name = f"execution-job-qir-{qir_name}-{device_name}-{jobname_suffix}"
    #         ref_execute_job = qnx.start_execute_job(
    #             programs=[qir_program_ref],
    #             n_shots=[1000],
    #             backend_config=config,
    #             name=job_name,
    #         )

    #         qnx.jobs.wait_for(ref_execute_job)
    #         qir_result = qnx.jobs.results(ref_execute_job)[0].download_result()
    #         counts_qir = self.preprocess_counts(qir_result.get_counts())

    #         # Run pytket circuit normally
    #         backend = AerBackend()
    #         uncompiled_pytket_circ = backend.get_compiled_circuit(
    #             circuit.copy(), optimisation_level=0
    #         )
    #         handle = backend.process_circuit(uncompiled_pytket_circ, n_shots=1000)
    #         result_pytket = backend.get_result(handle)
    #         counts_pytket = self.preprocess_counts(result_pytket.get_counts())

    #         # Run the kstest on the two results
    #         ks_value = self.ks_test(counts_qir, counts_pytket, 1000)
    #         print(f"QIR vs Pytket ks-test p-value: {ks_value}")

    #         if ks_value < 0.05:
    #             print(f"Interesting circuit found: {circuit_number}")
    #             self.save_interesting_circuit(
    #                 circuit_number, self.OUTPUT_DIR / "interesting_circuits"
    #             )

    #     except Exception as e:
    #         print("Error during QIR conversion or execution:", e)
    #         print("Exception :", traceback.format_exc())
    #         print("Exception :", traceback.format_exc())
    #         print("Exception :", traceback.format_exc())
    #         print("Exception :", traceback.format_exc())
    #         print("Exception :", traceback.format_exc())
    #         print("Exception :", traceback.format_exc())
    #         print("Exception :", traceback.format_exc())
    #         print("Exception :", traceback.format_exc())
