CMAKE ?= cmake
BUILD ?= build
BONUS ?= ON
JOBS ?= 4
.PHONY: all bonus test format-check clean fclean re
all:
	$(CMAKE) -S . -B $(BUILD) -DCMAKE_BUILD_TYPE=Release -DRAYTRACER_BONUS=$(BONUS)
	$(CMAKE) --build $(BUILD) -j $(JOBS)
bonus:
	$(MAKE) all BONUS=ON
test: all
	ctest --test-dir $(BUILD) --output-on-failure
format-check:
	python3 tools/check_source_layout.py .
clean:
	$(CMAKE) -E rm -rf $(BUILD)
fclean: clean
	$(CMAKE) -E rm -f raytracer plugins/raytracer_example.so
re: fclean
	$(MAKE) all
