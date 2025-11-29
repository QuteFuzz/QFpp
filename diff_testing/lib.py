"""
    Minimal differential testing library
    
    There are 2 forms of differential testing:
    - probability distribution comparison by ks-test after running on simulator
    - statvector comparison

    What is being tested?
    - results after running circuit through specific compiler passes
    - results after running circuit through compiler at different optimisation levels

    Results:
    - probability distributions plotted
    - ks value printed in logs
"""

from typing import List, Tuple, Any
from collections import Counter
from itertools import zip_longest
from scipy.stats import ks_2samp
from numpy.typing import NDArray
import numpy as np
import sys
import os
import datetime
import argparse
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import traceback
import pathlib
import shutil
from concurrent.futures import ThreadPoolExecutor, TimeoutError as FuturesTimeoutError

# Pytket imports
from pytket.circuit import Circuit
from pytket.passes import *
from pytket.extensions.qiskit import AerBackend
from pytket.extensions.qiskit import AerStateBackend

# Qiskit imports
from qiskit import QuantumCircuit, transpile

# Guppylang imports
from guppylang import guppy
from guppylang import enable_experimental_features
from guppylang.std.quantum import *
from guppylang.std.qsystem import *
from guppylang.std.builtins import result, array
enable_experimental_features()
from selene_sim import build, Quest
from hugr.qsystem.result import QsysResult

# QIR imports
from hugr_qir.hugr_to_qir import hugr_to_qir
from qirrunner import run, OutputHandler
from pytket.qir.conversion.api import pytket_to_qir, QIRFormat
import qnexus as qnx
import pyqir

class Base():
    # Define the plots directory as a class variable
    OUTPUT_DIR = (pathlib.Path(__file__).parent.parent / "outputs").resolve()
    # Define timeout seconds for any compilation
    TIMEOUT_SECONDS = 30

    def __init__(self):
        super().__init__()
        self.parser = argparse.ArgumentParser()
        self.parser.add_argument('--plot', action='store_true',help='Plot results after running circuit')
        self.args = self.parser.parse_args()
        self.plot : bool = self.args.plot

    def qnexus_login(self) -> None:
        '''
        Logs into QNexus using environment variables for running QIR jobs
        '''
        user_email: str = os.getenv("NEXUS_USERNAME")
        user_password: str = os.getenv("NEXUS_PWD")
        try:
            qnx.client.auth.login_no_interaction(user_email, user_password)
        except Exception as e:
            print("Error logging into Nexus:", e)

    def qnexus_check_login_status(self) -> bool:
        '''
        Checks if logged into QNexus to prevent trying login multiple times
        '''
        try:
            qnx.teams.get_all()
            return True
        except Exception as e:
            return False

    def preprocess_counts(self, counts : Counter[Tuple[str, ...], int]) -> Counter[int, int]:
        '''
            Given a dict mapping binary values to number of times they appear, return a sorted dict with each binary tuple/string converted into a base 10 int
            Dict is sorted by key
        '''
        out : Counter[int, int] = {}
        for k in counts.keys():
            # k can be a tuple of bits or a string of bits
            if isinstance(k, tuple):
                key_str = ''.join(str(x) for x in k).replace(' ', '')
            else:
                key_str = str(k).replace(' ', '')
            out[int(key_str, 2)] = counts[k]
        
        return dict(sorted(out.items()))
    
    def ks_test(self, counts1 : Counter[int, int], counts2 : Counter[int, int], total_shots : int) -> float:
        '''
        Carries out K-S test on two frequency lists
        '''
        sample1, sample2 = [], []

        for p1, p2 in zip_longest(counts1.items(), counts2.items(), fillvalue=None):
            if(p1):
                sample1 += p1[1] * [p1[0]]
            
            if(p2):
                sample2 += p2[1] * [p2[0]]

        assert (len(sample1) == total_shots) and (len(sample2) == total_shots), "Sample size does not match number of shots"

        ks_stat, p_value = ks_2samp(sorted(sample1), sorted(sample2), method='asymp')

        return p_value
    
    def compare_statevectors(self, sv1 : NDArray[np.complex128], sv2 : NDArray[np.complex128], precision: int = 6) -> float:
        return np.round(abs(np.vdot(sv1, sv2)), precision)

    def save_interesting_circuit(self, circuit_number: int, interesting_dir: pathlib.Path) -> None:
        '''
        Saves an interesting circuit file to the specified directory
        '''

        interesting_dir.mkdir(parents=True, exist_ok=True)
        
        circuit_source_path = self.OUTPUT_DIR / f"circuit{circuit_number}" / "circuit.py"
        circuit_dest_path = interesting_dir / f"circuit{circuit_number}.py"
        
        if circuit_source_path.exists():
            try:
                shutil.copy2(circuit_source_path, circuit_dest_path)
                print(f"Interesting circuit saved to: {circuit_dest_path}")
            except Exception as e:
                print(f"Error copying circuit file: {e}")
        else:
            print(f"Warning: Circuit file not found at {circuit_source_path}")

    def plot_histogram(self, res : Counter[int, int], title : str, compilation_level : int, circuit_number : int = 0):
        plots_dir = self.OUTPUT_DIR / f"circuit{circuit_number}"
        if not plots_dir.exists():
            plots_dir.mkdir(parents=True, exist_ok=True)
        
        plots_path = plots_dir /f"output{circuit_number}_{title}{compilation_level if compilation_level else 'uncompiled'}.png"
        
        # Plot the histogram
        values = list(res.keys())
        freqs = list(res.values())

        bar_width = 0.5
        plt.bar(values, freqs, width=bar_width, edgecolor='black')
        ax = plt.gca()
        ax.xaxis.set_major_locator(ticker.MaxNLocator(nbins=10, integer=True))
        plt.xlabel("Possible results")
        plt.ylabel("Number of occurances")
        plt.title(title)
        plt.tight_layout()  # Adjust layout to prevent clipping of ylabel and title
        plt.savefig(plots_path)
        plt.close()  # Close the plot to free up memory

class pytketTesting(Base):
    def __init__(self):
        super().__init__()
    
    def ks_diff_test(self, circuit : Circuit, circuit_number : int) -> None:
        '''
        Runs circuit on pytket simulator and returns counts
        '''
        backend = AerBackend()

        try:
            # Get original circuit shots
            uncompiled_circ = backend.get_compiled_circuit(circuit, optimisation_level=0)
            handle1 = backend.process_circuit(uncompiled_circ, n_shots=100000)
            result1 = backend.get_result(handle1)
            counts1 = self.preprocess_counts(result1.get_counts())
            is_testcase_interesting = False
            consistency_counter = 0

            # Compile circuit at 3 different optimisation levels
            for i in range(3):
                compiled_circ = backend.get_compiled_circuit(circuit, optimisation_level=i+1)
            
                # Process the compiled circuit
                handle2 = backend.process_circuit(compiled_circ, n_shots=100000)
                result2 = backend.get_result(handle2)
                counts2 = self.preprocess_counts(result2.get_counts())

                # Run the kstest on the two results
                ks_value = self.ks_test(counts1, counts2, 100000)
                print(f"Optimisation level {i+1} ks-test p-value: {ks_value}")

                # Heuristic to determine if the testcase is interesting
                if ks_value < 0.2 :
                    consistency_counter += 1
                if ks_value < 0.05 or consistency_counter >= 2:
                    is_testcase_interesting = True
                
                # plot results
                if self.plot:
                    self.plot_histogram(counts1, "Uncompiled Circuit Results", 0, circuit_number)
                    self.plot_histogram(counts2, "Compiled Circuit Results", i+1, circuit_number)

        except Exception as e:
            print("Exception :", traceback.format_exc())
            self.save_interesting_circuit(circuit_number, self.OUTPUT_DIR / "interesting_circuits")

        # Dump files to a "interesting circuits" folder if found interesting testcase
        if is_testcase_interesting:
            print(f"Interesting circuit found: {circuit_number}")
            self.save_interesting_circuit(circuit_number, self.OUTPUT_DIR / "interesting_circuits")

    def run_circ_statevector(self, circuit : Circuit, circuit_number : int) -> NDArray[np.complex128]:
        '''
        Runs circuit on pytket simulator and returns statevector
        '''
        try:
            backend = AerStateBackend()
            # circuit with no passes
            uncompiled_circ = backend.get_compiled_circuit(circuit.copy(), optimisation_level=0)
            no_pass_statevector = uncompiled_circ.get_statevector()

            # Get statevector after every optimisation level
            for i in range(3):
                compiled_circ = backend.get_compiled_circuit(circuit.copy(), optimisation_level=i+1)
                pass_statevector = compiled_circ.get_statevector()
                dot_prod = self.compare_statevectors(no_pass_statevector, pass_statevector, 6)

                if dot_prod == 1:
                    print("Statevectors are the same\n")
                else:
                    print ("Statevectors not the same")
                    print("Dot product: ", dot_prod)

        except Exception:
            print("Exception :", traceback.format_exc())

    def run_guppy_pytket_diff(self, circuit : Circuit, circuit_number : int, qubit_defs_list : list[int], bit_defs_list : list[int]) -> None:
        '''
        Loads pytket circuit as a guppy circuit and runs it, comparing the results
        with normal AerBackend
        '''
        pytket_circ_copy = circuit.copy()

        # Define the guppy gateset
        from pytket import OpType
        guppy_gateset = { OpType.CX, OpType.CZ, OpType.CY, OpType.X, OpType.Y, OpType.Z, OpType.H, OpType.T,
                          OpType.Tdg, OpType.Rx, OpType.Ry, OpType.Rz, OpType.S, OpType.Sdg, OpType.CCX,
                            OpType.V, OpType.Vdg, OpType.CRz }
        DecomposeBoxes().apply(circuit) # Decompose circboxes for guppy compatibility
        AutoRebase(guppy_gateset).apply(circuit) # Rebase to guppy gateset

        guppy_circuit = guppy.load_pytket("guppy_circuit", circuit)

        # Reordering qubits in alphebatical order like in pytket
        qubit_defs_list_sorted =  [x[1] for x in sorted(enumerate(qubit_defs_list), key=lambda x: (x[1] == 0, x[0]))]
        # Reordering bits in alphebetical order like in pytket
        bit_defs_list_sorted = [x for x in bit_defs_list if x == 0] + [x for x in bit_defs_list if x != 0]
        # And if creg is the sole register, it will be expanded to individual bits
        if len(bit_defs_list_sorted) == 1 and bit_defs_list_sorted[0] != 0:
            bit_defs_list_sorted = [1] * bit_defs_list_sorted[0]
        
        @guppy.comptime
        def main() -> None:
            qubit_variables = []
            
            # qreg are always at the front, followed by singular qubits
            for qubit_def in qubit_defs_list_sorted:
                qubit_array = [qubit() for _ in range(qubit_def)] if qubit_def > 0 else [qubit()]
                qubit_variables.append(qubit_array)
            # At this point, bit and qubit ordering is different. Needs to be adapted here
            creg_results = guppy_circuit(*qubit_variables)

            for i in range(len(bit_defs_list_sorted)):
                if bit_defs_list_sorted[i] == 0:
                    result(f"b{i}", creg_results[i])
            for r in range(len(qubit_variables)):
                if isinstance(qubit_variables[r], list):
                    result(f"q{r}", measure_array(qubit_variables[r]))
            if creg_results is not None and hasattr(creg_results, '__len__'):
                for i in range(len(bit_defs_list_sorted)):
                    if bit_defs_list_sorted[i] != 0:
                        result(f"creg{i}", creg_results[i])

        try:
            # Getting guppy circuit results
            compiled_circ = main.compile()
            runner = build(compiled_circ)
            results = QsysResult(
                runner.run_shots(Quest(), n_qubits=circuit.n_qubits, n_shots=10000)
            )
            counts_guppy = results.collated_counts()
            counts_guppy = Counter({''.join([measurement[1] for measurement in key]): value for key, value in counts_guppy.items()})
            counts_guppy = self.preprocess_counts(counts_guppy)
            print("Processed guppy counts:", counts_guppy)

            # Getting uncompiled pytket circuit results
            backend = AerBackend()
            pytket_circ_copy.measure_all()
            uncompiled_pytket_circ = backend.get_compiled_circuit(pytket_circ_copy, optimisation_level=0)
            handle = backend.process_circuit(uncompiled_pytket_circ, n_shots=10000)
            result_pytket = backend.get_result(handle)
            counts_pytket = self.preprocess_counts(result_pytket.get_counts())
            print("Pytket counts:", counts_pytket)

            # Run the kstest on the two results
            ks_value = self.ks_test(counts_guppy, counts_pytket, 10000)
            print(f"Guppy vs Pytket ks-test p-value: {ks_value}")

            # Heuristic to determine if the testcase is interesting
            if ks_value < 0.05:
                print(f"Interesting circuit found: {circuit_number}")
                self.save_interesting_circuit(circuit_number, self.OUTPUT_DIR / "interesting_circuits")
            
            if self.plot:
                self.plot_histogram(counts_guppy, "Guppy Circuit Results", 0, circuit_number)
                self.plot_histogram(counts_pytket, "Pytket Circuit Results", 0, circuit_number)

        except Exception as e:
            from guppylang_internals.error import GuppyError
            if isinstance(e, GuppyError):
                from guppylang_internals.diagnostic import DiagnosticsRenderer
                from guppylang_internals.engine import DEF_STORE

                renderer = DiagnosticsRenderer(DEF_STORE.sources)
                renderer.render_diagnostic(e.error)
                sys.stderr.write("\n".join(renderer.buffer))
                sys.stderr.write("\n\nGuppy compilation failed due to 1 previous error\n")
            else:
                # If it's not a GuppyError, fall back to default hook
                print("Error during compilation:", e)
                print("Exception :", traceback.format_exc())

    def run_qir_pytket_diff(self, circuit: Circuit, circuit_number: int) -> None:
        '''
        Loads pytket circuit as qir and runs it, comparing the results
        '''
        if not self.qnexus_check_login_status():
            self.qnexus_login()

        try:
            # Convert pytket circuit to QIR
            qir_circuit = circuit.copy()
            # Flatten and relabel registers to ensure consistent naming
            FlattenRelabelRegistersPass().apply(qir_circuit)
            
            qir_LLVM = pytket_to_qir(qir_circuit, qir_format=QIRFormat.STRING)
            project = qnx.projects.get_or_create(name="qir_pytket_diff")
            qnx.context.set_active_project(project)
            qir_name = "pytket_qir_circuit"+str(circuit_number)
            jobname_suffix = datetime.datetime.now().strftime("%Y_%m_%d_%H_%M_%S")
            qir = pyqir.Module.from_ir(pyqir.Context(), qir_LLVM).bitcode
            qir_program_ref = qnx.qir.upload(qir=qir, name=qir_name, project=project)

            # Run the QIR on H1-Emulator emulator
            device_name = "H1-Emulator"

            qnx.context.set_active_project(project)
            config = qnx.QuantinuumConfig(device_name=device_name)

            job_name = f"execution-job-qir-{qir_name}-{device_name}-{jobname_suffix}"
            ref_execute_job = qnx.start_execute_job(
                programs=[qir_program_ref],
                n_shots=[1000],
                backend_config=config,
                name=job_name,
            )

            qnx.jobs.wait_for(ref_execute_job)
            qir_result = qnx.jobs.results(ref_execute_job)[0].download_result()
            counts_qir = self.preprocess_counts(qir_result.get_counts())

            # Run pytket circuit normally
            backend = AerBackend()
            uncompiled_pytket_circ = backend.get_compiled_circuit(circuit.copy(), optimisation_level=0)
            handle = backend.process_circuit(uncompiled_pytket_circ, n_shots=1000)
            result_pytket = backend.get_result(handle)
            counts_pytket = self.preprocess_counts(result_pytket.get_counts())

            # Run the kstest on the two results
            ks_value = self.ks_test(counts_qir, counts_pytket, 1000)
            print(f"QIR vs Pytket ks-test p-value: {ks_value}")

            if ks_value < 0.05:
                print(f"Interesting circuit found: {circuit_number}")
                self.save_interesting_circuit(circuit_number, self.OUTPUT_DIR / "interesting_circuits")

        except Exception as e:
            print("Error during QIR conversion or execution:", e)
            print("Exception :", traceback.format_exc())

class qiskitTesting(Base):
    def __init__(self):
        super().__init__()

    def ks_diff_test(self, circuit : Circuit, circuit_number : int) -> float:
        '''
        Runs circuit on qiskit simulator and returns counts
        '''

        from qiskit_aer import AerSimulator
        backend = AerSimulator()
        
        # Get original circuit shots
        circuit.measure_all()
        uncompiled_circ = transpile(circuit, backend, optimization_level=0)
        counts1 = self.preprocess_counts(backend.run(uncompiled_circ, shots=10000).result().get_counts())

        # Compile circuit at 3 different optimisation levels
        for i in range(3):
            compiled_circ = transpile(circuit, backend, optimization_level=i+1)
            counts2 = self.preprocess_counts(backend.run(compiled_circ, shots=10000).result().get_counts())

            # Run the kstest on the two results
            ks_value = self.ks_test(counts1, counts2, 10000)
            print(f"Optimisation level {i+1} ks-test p-value: {ks_value}")

            if ks_value < 0.05:
                print(f"Interesting circuit found: {circuit_number}")
                self.save_interesting_circuit(circuit_number, self.OUTPUT_DIR / "interesting_circuits")

            # plot results
            if self.plot:
                self.plot_histogram(counts1, "Uncompiled Circuit Results", 0, circuit_number)
                self.plot_histogram(counts2, "Compiled Circuit Results", i+1, circuit_number)

        return ks_value
        
    
class cirqTesting(Base):
    def __init__(self):
        super().__init__()
    
    def run_circ(self, circuit : Circuit) -> Counter[int, int]:
        '''
        Runs circuit on cirq simulator and returns counts
        '''


class guppyTesting(Base):
    def __init__(self):
        super().__init__()

    def ks_diff_test(self, circuit : Any, circuit_number : int) -> None:
        '''
        Compile guppy circuit into hugr and optimise through TKET for differential testing
        '''        
        def compile_circuit():
            return circuit.compile()
        
        # Run the compile with timeout using ThreadPoolExecutor
        try:
            with ThreadPoolExecutor(max_workers=1) as executor:
                future = executor.submit(compile_circuit)
                result = future.result(timeout=self.TIMEOUT_SECONDS)

        except FuturesTimeoutError:
            print(f"Compilation timed out after {self.TIMEOUT_SECONDS} seconds")
            self.save_interesting_circuit(circuit_number, self.OUTPUT_DIR / "interesting_circuits")
        except Exception as e:
            from guppylang_internals.error import GuppyError
            if isinstance(e, GuppyError):
                from guppylang_internals.diagnostic import DiagnosticsRenderer
                from guppylang_internals.engine import DEF_STORE

                renderer = DiagnosticsRenderer(DEF_STORE.sources)
                renderer.render_diagnostic(e.error)
                sys.stderr.write("\n".join(renderer.buffer))
                sys.stderr.write("\n\nGuppy compilation failed due to 1 previous error\n")
                return

            # If it's not a GuppyError, fall back to default hook
            print("Error during compilation:", e)
            print("Exception :", traceback.format_exc())
            self.save_interesting_circuit(circuit_number, self.OUTPUT_DIR / "interesting_circuits")

        # TODO: Insert TKET optimisation passes here
            

    def guppy_qir_diff_test(self, circuit : Any, circuit_number : int, total_num_qubits : int) -> None:
        '''
        Compile guppy circuit into hugr and convert to QIR for differential testing
        '''

        if not self.qnexus_check_login_status():
            self.qnexus_login()

        try:
            hugr = circuit.compile()
            # Circuit compiled successfully, now differential test hugr
            # Running hugr on selene
            runner = build(hugr)
            results = QsysResult(
                runner.run_shots(Quest(), n_qubits=total_num_qubits, n_shots=1000)
            )
            counts_guppy = results.collated_counts()
            counts_guppy = Counter({''.join([measurement[1] for measurement in key]): value for key, value in counts_guppy.items()})
            counts_guppy = self.preprocess_counts(counts_guppy)

            qir_LLVM = hugr_to_qir(hugr, emit_text=True)
            project = qnx.projects.get_or_create(name="guppy_qir_diff")
            qnx.context.set_active_project(project)
            qir_name = "guppy_qir_circuit"+str(circuit_number)
            jobname_suffix = datetime.datetime.now().strftime("%Y_%m_%d_%H_%M_%S")
            qir = pyqir.Module.from_ir(pyqir.Context(), qir_LLVM).bitcode
            qir_program_ref = qnx.qir.upload(qir=qir, name=qir_name, project=project)

            # Run the QIR on H1-Emulator
            device_name = "H1-Emulator"

            qnx.context.set_active_project(project)
            config = qnx.QuantinuumConfig(device_name=device_name)

            job_name = f"execution-job-qir-{qir_name}-{device_name}-{jobname_suffix}"
            ref_execute_job = qnx.start_execute_job(
                programs=[qir_program_ref],
                n_shots=[1000],
                backend_config=config,
                name=job_name,
            )

            qnx.jobs.wait_for(ref_execute_job)
            qir_result = qnx.jobs.results(ref_execute_job)[0].download_result()
            counts_qir = self.preprocess_counts(qir_result.get_counts())

            # Run the kstest on the two results
            ks_value = self.ks_test(counts_guppy, counts_qir, 1000)
            print(f"Guppy vs QIR ks-test p-value: {ks_value}")

            if ks_value < 0.05:
                print(f"Interesting circuit found: {circuit_number}")
                self.save_interesting_circuit(circuit_number, self.OUTPUT_DIR / "interesting_circuits")

            if self.plot:
                self.plot_histogram(counts_guppy, "Guppy Circuit Results", 0, circuit_number)
                self.plot_histogram(counts_qir, "Guppy-QIR Circuit Results", 0, circuit_number)
                
        except Exception as e:
            from guppylang_internals.error import GuppyError
            if isinstance(e, GuppyError):
                from guppylang_internals.diagnostic import DiagnosticsRenderer
                from guppylang_internals.engine import DEF_STORE

                renderer = DiagnosticsRenderer(DEF_STORE.sources)
                renderer.render_diagnostic(e.error)
                sys.stderr.write("\n".join(renderer.buffer))
                sys.stderr.write("\n\nGuppy compilation failed due to 1 previous error\n")
                return

            # If it's not a GuppyError, fall back to default hook
            print("Error during compilation:", e)
            print("Exception :", traceback.format_exc())
            self.save_interesting_circuit(circuit_number, self.OUTPUT_DIR / "interesting_circuits")

