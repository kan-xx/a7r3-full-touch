PYTHON ?= python3
ZIG ?= zig

.PHONY: all test build-core build-observers verify-unicorn clean

all: test

test:
	$(PYTHON) tests/run_all_tests.py

build-core:
	@if [ -z "$(A7R3_FW_ROOT)" ]; then \
		echo "Error: A7R3_FW_ROOT must point to your extracted A7R III FW 3.10 root"; \
		exit 1; \
	fi
	$(PYTHON) touch-core/build_core.py --fw-root "$(A7R3_FW_ROOT)" --zig "$(ZIG)" --out-dir build

build-observers:
	$(PYTHON) observers/build_observers.py --zig "$(ZIG)" $(if $(A7R3_FW_ROOT),--fw-root "$(A7R3_FW_ROOT)",) --out-dir build/observers

verify-unicorn:
	@if [ -z "$(A7R3_FW_ROOT)" ]; then \
		echo "Error: A7R3_FW_ROOT must point to your extracted A7R III FW 3.10 root"; \
		exit 1; \
	fi
	$(PYTHON) verifiers/widget_mode_arm_test.py --fw-root "$(A7R3_FW_ROOT)"
	$(PYTHON) verifiers/widget_event_filter_arm_test.py --fw-root "$(A7R3_FW_ROOT)"
	$(PYTHON) verifiers/verify_native_fn_direction.py --fw-root "$(A7R3_FW_ROOT)"
	$(PYTHON) verifiers/verify_native_table.py --fw-root "$(A7R3_FW_ROOT)"
	$(PYTHON) verifiers/verify_native.py --fw-root "$(A7R3_FW_ROOT)"

clean:
	rm -rf build dist *.egg-info
	find . -name "__pycache__" -type d -prune -exec rm -rf {} +
	find . -name "*.pyc" -delete
