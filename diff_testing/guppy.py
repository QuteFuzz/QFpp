import datetime
import sys
import traceback
from concurrent.futures import ThreadPoolExecutor
from typing import Any, Dict

import pyqir
import qnexus as qnx
from guppylang import enable_experimental_features
from guppylang_internals.diagnostic import DiagnosticsRenderer
from guppylang_internals.engine import DEF_STORE
from guppylang_internals.error import GuppyError
from hugr.qsystem.result import QsysResult
from qnexus.qir.conversion.hugr import hugr_to_qir  # type: ignore
from selene_sim import Quest
from selene_sim.build import build

from .lib import Base

enable_experimental_features()


class guppyTesting(Base):
    def __init__(self, native=True) -> None:
        super().__init__("guppy", native)

    def ks_diff_test(self, circuit: Any, circuit_number: int) -> None:
        """
        Compile guppy circuit into hugr and optimise through TKET for differential testing
        """

        def compile_circuit() -> Any:
            return circuit.compile()

        # Run the compile with timeout using ThreadPoolExecutor
        with ThreadPoolExecutor(max_workers=1) as executor:
            future = executor.submit(compile_circuit)
            # Wait for result, ignoring return value as we just test compilation here?
            # Original code assigned result but didn't use it immediately except for potential
            # future passes
            _ = future.result(timeout=self.TIMEOUT_SECONDS)

        # TODO: Insert TKET optimisation passes here

    def guppy_qir_diff_test(self, circuit: Any, circuit_number: int, total_num_qubits: int) -> None:
        """
        Compile guppy circuit into hugr and convert to QIR for differential testing
        """

        if not self.qnexus_check_login_status():
            self.qnexus_login()

        try:
            hugr = circuit.compile()
            # Circuit compiled successfully, now differential test hugr
            # Running hugr on selene
            runner = build(hugr)
            results = QsysResult(runner.run_shots(Quest(), n_qubits=total_num_qubits, n_shots=1000))
            counts_guppy = results.collated_counts()

            reconstructed_counts: Dict[Any, int] = {
                "".join([measurement[1] for measurement in key]): value
                for key, value in counts_guppy.items()
            }

            processed_counts_guppy = self.preprocess_counts(reconstructed_counts, total_num_qubits)

            qir_LLVM = hugr_to_qir(hugr, emit_text=True)
            project = qnx.projects.get_or_create(name="guppy_qir_diff")
            qnx.context.set_active_project(project)
            qir_name = "guppy_qir_circuit" + str(circuit_number)
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
            qir_result = qnx.jobs.results(ref_execute_job)[0].download_result()  # type: ignore
            counts_qir = self.preprocess_counts(qir_result.get_counts())  # type: ignore

            # Run the kstest on the two results
            ks_value = self.ks_test(processed_counts_guppy, counts_qir)
            print(f"Guppy vs QIR ks-test p-value: {ks_value}")

            if self.plot:
                self.plot_histogram(processed_counts_guppy, "Guppy Circuit Results", circuit_number)
                self.plot_histogram(counts_qir, "Guppy-QIR Circuit Results", circuit_number)

        except Exception as e:
            if isinstance(e, GuppyError):
                renderer = DiagnosticsRenderer(DEF_STORE.sources)
                renderer.render_diagnostic(e.error)
                sys.stderr.write("\n".join(renderer.buffer))
                sys.stderr.write("\n\nGuppy compilation failed due to 1 previous error\n")
                return

            # If it's not a GuppyError, fall back to default hook
            print("Error during compilation:", e)
            print("Exception :", traceback.format_exc())
