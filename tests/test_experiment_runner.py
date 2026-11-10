"""Integration checks: python tests/test_experiment_runner.py [path/to/main.exe]."""
import csv
import io
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
EXECUTABLE = Path(sys.argv.pop(1)).resolve() if len(sys.argv) > 1 else ROOT / "main.exe"
ALGORITHMS = {"tsp", "greedy", "exact-partitioning", "hgvns", "hga-tac", "hga-tac-plus",
              "hgvns-pd-waiting-rvnd"}


def instance(size=5):
    return (
        "/*The speed of the Truck*/\n1.0\n"
        "/*The speed of the Drone*/\n0.5\n"
        f"/*Number of Nodes*/\n{size}\n"
        "/*The Depot*/\n0 0 depot\n"
        "/*The Locations (x_coor y_coor name)*/\n"
        + "".join(f"{i} {i % 3} v{i}\n" for i in range(1, size))
    )


class RunnerTests(unittest.TestCase):
    def setUp(self):
        build = ROOT / "build"
        build.mkdir(exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="runner-tests-", dir=build)
        self.addCleanup(self.temp.cleanup)
        self.work = Path(self.temp.name)
        data = self.work / "data"
        (data / "singlecenter" / "deep").mkdir(parents=True)
        (data / "singlecenter" / "solutions").mkdir()
        (data / "uniform").mkdir()
        for name in ["singlecenter/a,1.txt", "singlecenter/deep/b.TXT", "uniform/c.txt"]:
            (data / name).write_text(instance(), encoding="utf-8")
        # A valid-looking file inside solutions must still never be selected.
        (data / "singlecenter/solutions/reference.txt").write_text(instance())
        (data / "notes.txt").write_text("auxiliary text")
        (data / "README.md").write_text("readme")
        (data / "archive.zip").write_bytes(b"not an instance")

    def run_solver(self, *args, code=0):
        before = set((self.work / "results").glob("rodada-*.csv"))
        result = subprocess.run(
            [str(EXECUTABLE), *args], cwd=self.work, capture_output=True,
            encoding="utf-8", errors="replace", timeout=30,
        )
        self.assertEqual(result.returncode, code, result.stdout + result.stderr)
        after = set((self.work / "results").glob("rodada-*.csv"))
        created = after - before
        self.assertLessEqual(len(created), 1)
        rows = []
        self.analysis_rows = []
        if created:
            report = created.pop()
            analysis = report.with_name(report.name.replace("rodada-", "analise-", 1))
            self.assertTrue(analysis.exists())
            self.analysis_rows = list(csv.DictReader(io.StringIO(analysis.read_text(encoding="utf-8"))))
            reader = csv.DictReader(io.StringIO(report.read_text(encoding="utf-8")))
            self.assertEqual(reader.fieldnames, [
                "name", "algorithm", "tspd_cost", "tsp_cost", "literature_tspd_cost",
                "literature_tsp_cost", "solve_seconds", "status", "error",
            ])
            rows = list(reader)
            for row in rows:
                self.assertGreaterEqual(float(row["solve_seconds"]), 0)
        return result, rows

    def test_root_recursive_all_algorithms(self):
        result, rows = self.run_solver("data/")
        self.assertIn("Graficos: nao", result.stdout)
        self.assertEqual(len(rows), 3 * len(ALGORITHMS))
        paths = list(dict.fromkeys(row["name"] for row in rows))
        self.assertEqual(paths, sorted(paths))
        self.assertEqual(len(paths), 3)
        self.assertTrue(all(row["status"] == "success" for row in rows))
        for row in rows:
            self.assertNotIn("/", row["name"])
            self.assertNotIn("\\", row["name"])
            self.assertTrue(row["tspd_cost"])
            self.assertTrue(row["tsp_cost"])
            self.assertEqual(row["literature_tspd_cost"], "")
            self.assertEqual(row["literature_tsp_cost"], "")
        self.assertEqual({row["algorithm"] for row in rows}, ALGORITHMS)
        self.assertEqual(result.stdout.count("Optimization TSP Solver Initialized."), 3)

    def test_subdirectory_and_one_algorithm(self):
        _, rows = self.run_solver("data/singlecenter", "--algorithms", "greedy")
        self.assertEqual(len(rows), 2)
        self.assertTrue(all(row["algorithm"] == "greedy" for row in rows))

    def test_single_file_no_graphics(self):
        result, rows = self.run_solver("data/singlecenter/a,1.txt", "--no-graphics")
        self.assertIn("Graficos: nao", result.stdout)
        self.assertEqual(len(rows), len(ALGORITHMS))
        self.assertEqual({row["name"] for row in rows}, {"a,1.txt"})

    def test_algorithm_selection_order_duplicates_and_independence(self):
        _, baseline = self.run_solver("data/uniform/c.txt", "--no-graphics")
        baseline = [row for row in baseline if row["algorithm"] in {"greedy", "exact-partitioning"}]
        _, rows = self.run_solver(
            "--algorithms", "exact-partitioning,greedy,greedy", "--no-graphics", "data/uniform/c.txt"
        )
        self.assertEqual([row["algorithm"] for row in rows], ["greedy", "exact-partitioning"])
        self.assertEqual([row["tspd_cost"] for row in baseline], [row["tspd_cost"] for row in rows])
        _, alone = self.run_solver(
            "--no-graphics", "data/uniform/c.txt", "--algorithms", "exact-partitioning"
        )
        self.assertEqual(len(alone), 1)
        self.assertEqual(alone[0]["tspd_cost"], baseline[1]["tspd_cost"])

    def test_read_failure_continues(self):
        (self.work / "data/singlecenter/0-broken.txt").write_text(
            "/*The speed of the Truck*/\n1\n/*Number of Nodes*/\ninvalid\n"
        )
        result, rows = self.run_solver("data/", code=1)
        self.assertEqual(len(rows), 4 * len(ALGORITHMS))
        self.assertEqual(sum(row["status"] == "error" for row in rows), len(ALGORITHMS))
        self.assertEqual(sum(row["status"] == "success" for row in rows), 3 * len(ALGORITHMS))
        self.assertIn("Com falhas: 1", result.stdout)
        for row in rows:
            if row["status"] == "error":
                self.assertEqual(row["tspd_cost"], "")
                self.assertEqual(row["tsp_cost"], "")
                self.assertEqual(row["literature_tspd_cost"], "")
                self.assertEqual(row["literature_tsp_cost"], "")
                self.assertTrue(row["error"])

    def test_reference_costs_when_available(self):
        path = self.work / "data/singlecenter/triangle.txt"
        path.write_text(
            "/*The speed of the Truck*/\n1\n/*The speed of the Drone*/\n0.5\n"
            "/*Number of Nodes*/\n3\n/*The Depot*/\n0 0 depot\n"
            "/*The Locations (x_coor y_coor name)*/\n3 0 a\n0 4 b\n"
        )
        solutions = path.parent / "solutions"
        dp = solutions / "triangle-DP.txt"
        tsp = solutions / "triangle-tsp.txt"
        for with_dp, with_tsp in [(False, False), (False, True), (True, False), (True, True)]:
            with self.subTest(dp=with_dp, tsp=with_tsp):
                dp.unlink(missing_ok=True)
                tsp.unlink(missing_ok=True)
                if with_dp:
                    dp.write_text("/* Total cost: 8 */\n1\n0 0 2 1 1\n")
                if with_tsp:
                    tsp.write_text("1\n0 0 -1 2 1 2\n")
                _, rows = self.run_solver(str(path), "--no-graphics")
                self.assertEqual(len(rows), len(ALGORITHMS))
                analysis = self.analysis_rows[0]
                self.assertEqual(analysis["optimal_tspd_cost"], "")
                self.assertEqual(analysis["optimal_source"], "")
                self.assertEqual(analysis["literature_tspd_cost"], "8" if with_dp else "")
                for row in rows:
                    self.assertEqual(row["name"], "triangle.txt")
                    self.assertAlmostEqual(float(row["tsp_cost"]), 12.0)
                    if with_dp:
                        self.assertAlmostEqual(float(row["literature_tspd_cost"]), 8.0)
                    else:
                        self.assertEqual(row["literature_tspd_cost"], "")
                    if with_tsp:
                        self.assertAlmostEqual(float(row["literature_tsp_cost"]), 12.0)
                    else:
                        self.assertEqual(row["literature_tsp_cost"], "")

    def test_algorithm_failure_continues(self):
        (self.work / "data/singlecenter/0-large.txt").write_text(instance(21))
        route = " ".join(str(i) for i in range(1, 21))
        (self.work / "data/singlecenter/solutions/0-large-tsp.txt").write_text(
            f"1\n0 0 -1 20 {route}\n"
        )
        _, rows = self.run_solver("data/", code=1)
        self.assertEqual(len(rows), 4 * len(ALGORITHMS))
        failed = [row for row in rows if row["status"] == "error"]
        self.assertEqual(len(failed), 1)
        self.assertEqual(failed[0]["algorithm"], "exact-partitioning")
        self.assertIn("20", failed[0]["error"])
        self.assertEqual(failed[0]["tspd_cost"], "")
        self.assertEqual(failed[0]["tsp_cost"], "")
        self.assertEqual(failed[0]["literature_tspd_cost"], "")
        self.assertGreater(float(failed[0]["literature_tsp_cost"]), 0)
        self.assertEqual(rows[-1]["status"], "success")
        wide = next(r for r in self.analysis_rows if r["name"] == "0-large.txt")
        self.assertEqual(wide["exact-partitioning_result"], "")
        self.assertEqual(wide["exact-partitioning_status"], "error")
        self.assertIn("20", wide["exact-partitioning_error"])
        self.assertTrue(wide["tsp_result"])
        self.assertEqual(wide["hga-tac-plus_status"], "success")

    def test_reports_never_overwrite(self):
        self.run_solver("data/uniform", "--algorithms", "greedy")
        self.run_solver("data/uniform", "--algorithms", "greedy")
        self.assertEqual(len(list((self.work / "results").glob("rodada-*.csv"))), 2)
        self.assertEqual(len(list((self.work / "results").glob("analise-*.csv"))), 2)

    def test_invalid_arguments(self):
        invalid = [
            [], ["--no-graphics"], ["missing"], ["data", "--unknown"],
            ["data", "--algorithms"], ["data", "--algorithms", "--no-graphics"],
            ["data", "--algorithms", "unknown"], ["data", "--algorithms", ""],
            ["data", "--algorithms", "greedy,"], ["data", "--algorithms", ",greedy"],
            ["data", "--algorithms", "greedy,,exact-partitioning"], ["data", "data/uniform"],
            ["data", "--seed"], ["data", "--seed", "-1"], ["data", "--seed", "abc"],
            ["data", "--seed", "4294967296"], ["data", "--seed", ""],
        ]
        for args in invalid:
            with self.subTest(args=args):
                _, rows = self.run_solver(*args, code=1)
                self.assertEqual(rows, [])
        self.assertFalse((self.work / "results").exists())

    def test_empty_directory(self):
        (self.work / "empty").mkdir()
        result, rows = self.run_solver("empty", code=1)
        self.assertIn("Nenhuma instancia", result.stderr)
        self.assertEqual(rows, [])

    def test_unsupported_single_file(self):
        _, rows = self.run_solver("data/notes.txt", "--no-graphics", code=1)
        self.assertEqual(len(rows), len(ALGORITHMS))
        self.assertTrue(all(row["status"] == "error" for row in rows))

    def test_tsp_can_run_alone(self):
        result, rows = self.run_solver("data/uniform/c.txt", "--no-graphics", "--algorithms", "tsp")
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0]["algorithm"], "tsp")
        self.assertEqual(rows[0]["tspd_cost"], rows[0]["tsp_cost"])
        self.assertGreater(float(rows[0]["tsp_cost"]), 0)
        self.assertEqual(result.stdout.count("Optimization TSP Solver Initialized."), 1)

    def test_shared_tsp_runs_once_with_or_without_baseline_selected(self):
        for selected, expected in [
            ("exact-partitioning,tsp,greedy,tsp", ["tsp", "greedy", "exact-partitioning"]),
            ("greedy,exact-partitioning", ["greedy", "exact-partitioning"]),
            ("hgvns,greedy,tsp,exact-partitioning", ["tsp", "greedy", "exact-partitioning", "hgvns"]),
        ]:
            with self.subTest(algorithms=selected):
                result, rows = self.run_solver(
                    "data/uniform/c.txt", "--no-graphics", "--algorithms", selected
                )
                self.assertEqual([row["algorithm"] for row in rows], expected)
                self.assertEqual(result.stdout.count("Optimization TSP Solver Initialized."), 1)

    def test_depot_only_instance(self):
        path = self.work / "data/uniform/depot.txt"
        path.write_text(instance(1), encoding="utf-8")
        _, rows = self.run_solver(str(path), "--no-graphics")
        self.assertEqual(len(rows), len(ALGORITHMS))
        for row in rows:
            self.assertEqual(row["status"], "success")
            self.assertEqual(float(row["tsp_cost"]), 0)
            self.assertEqual(float(row["tspd_cost"]), 0)

    def test_hgvns_block_variants(self):
        selected = "hgvns,hgvns-pd-waiting-rvnd"
        _, rows = self.run_solver("data/uniform/c.txt", "--no-graphics", "--algorithms", selected, "--seed", "42")
        _, repeated = self.run_solver("data/uniform/c.txt", "--no-graphics", "--algorithms", selected, "--seed", "42")
        self.assertEqual([r["algorithm"] for r in rows], selected.split(","))
        self.assertTrue(all(r["status"] == "success" for r in rows))
        self.assertEqual([r["tspd_cost"] for r in rows], [r["tspd_cost"] for r in repeated])

    def test_hgvns_pd_placements(self):
        for removed in ("hgvns-pd-waiting", "hgvns-pd-random", "hgvns-pd-waiting-initial",
                        "hgvns-pd-random-initial", "hgvns-pd-waiting-shake",
                        "hgvns-pd-random-shake", "hgvns-pd-random-rvnd"):
            result, rows = self.run_solver("data/uniform/c.txt", "--algorithms", removed, code=1)
            self.assertEqual(rows, [])
            self.assertIn("Algoritmo desconhecido", result.stderr)

    def test_analysis_all_methods_and_tsp(self):
        _, details = self.run_solver("data/uniform/c.txt", "--no-graphics", "--seed", "42")
        self.assertEqual(len(self.analysis_rows), 1)
        row = self.analysis_rows[0]
        self.assertEqual(row["nodes"], "5")
        self.assertEqual(row["seed"], "42")
        self.assertEqual(row["optimal_tspd_cost"], "")
        self.assertEqual(row["optimal_source"], "")
        by_algorithm = {r["algorithm"]: r for r in details}
        self.assertEqual(row["tsp_result"], by_algorithm["tsp"]["tspd_cost"])
        self.assertEqual(row["tsp_seconds"], by_algorithm["tsp"]["solve_seconds"])
        for method in ALGORITHMS - {"tsp"}:
            self.assertEqual(row[method + "_result"], by_algorithm[method]["tspd_cost"])
            self.assertEqual(row[method + "_seconds"], by_algorithm[method]["solve_seconds"])
            self.assertEqual(row[method + "_status"], "success")
            self.assertAlmostEqual(float(row[method + "_total_seconds"]),
                                   float(row["tsp_seconds"]) + float(row[method + "_seconds"]))

    def test_analysis_optimum_is_explicit_and_unselected_is_empty(self):
        path = self.work / "data/uniform/c.txt"
        solutions = path.parent / "solutions"
        solutions.mkdir()
        (solutions / "c-optimal.txt").write_text("5.75\n")
        self.run_solver(str(path), "--no-graphics", "--algorithms", "greedy")
        row = self.analysis_rows[0]
        self.assertEqual(row["optimal_tspd_cost"], "5.75")
        self.assertTrue(row["optimal_source"].endswith("c-optimal.txt"))
        self.assertTrue(row["tsp_result"])
        self.assertTrue(row["tsp_seconds"])
        self.assertEqual(row["hgvns_result"], "")
        self.assertEqual(row["hgvns_seconds"], "")
        self.assertEqual(row["hgvns_status"], "not_selected")
        (solutions / "c-optimal.txt").write_text("invalid")
        self.run_solver(str(path), "--no-graphics", "--algorithms", "greedy", code=1)
        row = self.analysis_rows[0]
        self.assertEqual(row["tsp_result"], "")
        self.assertEqual(row["greedy_result"], "")
        self.assertEqual(row["greedy_status"], "error")

    def test_hgvns_seed_and_shared_initial_solution(self):
        first, rows = self.run_solver("data/uniform/c.txt", "--no-graphics", "--algorithms", "hgvns", "--seed", "42")
        _, repeated = self.run_solver("data/uniform/c.txt", "--no-graphics", "--algorithms", "hgvns", "--seed", "42")
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0]["algorithm"], "hgvns")
        self.assertEqual(rows[0]["tspd_cost"], repeated[0]["tspd_cost"])
        self.assertEqual(rows[0]["tsp_cost"], repeated[0]["tsp_cost"])
        self.assertEqual(first.stdout.count("Optimization TSP Solver Initialized."), 1)
        self.assertIn("Semente HGVNS: 42", first.stdout)

    def test_help(self):
        result, rows = self.run_solver("--help")
        self.assertIn("--algorithms", result.stdout)
        self.assertIn("--no-graphics", result.stdout)
        self.assertEqual(rows, [])

    def test_hga_tac_variants_seed_and_shared_tsp(self):
        for algorithm in ("hga-tac", "hga-tac-plus"):
            with self.subTest(algorithm=algorithm):
                result, rows = self.run_solver("data/uniform/c.txt", "--no-graphics",
                                              "--algorithms", algorithm, "--seed", "42")
                _, repeated = self.run_solver("data/uniform/c.txt", "--no-graphics",
                                             "--algorithms", algorithm, "--seed", "42")
                self.assertEqual(len(rows), 1)
                self.assertEqual(rows[0]["algorithm"], algorithm)
                self.assertEqual(rows[0]["status"], "success")
                self.assertEqual(rows[0]["tspd_cost"], repeated[0]["tspd_cost"])
                self.assertEqual(rows[0]["tsp_cost"], repeated[0]["tsp_cost"])
                self.assertEqual(result.stdout.count("Optimization TSP Solver Initialized."), 1)
                self.assertIn("Semente HGA-TAC: 42", result.stdout)


if __name__ == "__main__":
    unittest.main()
