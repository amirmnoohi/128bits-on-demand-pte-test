# Colors for output
GREEN := \033[0;32m
RED := \033[0;31m
YELLOW := \033[0;33m
BLUE := \033[0;34m
BOLD := \033[1m
NC := \033[0m # No Color

.PHONY: all clean test test_1 test_2 test_3 test_4 test_5 test_6 test_7 test_8 test_9 test_10 help

DIRS := test1 test2 test3 test4 test5 test6 test7 test8 test9 test10

define print_header
	@printf "${BLUE}${BOLD}=============================================\n"
	@printf "${BLUE}${BOLD}  $(1)\n"
	@printf "${BLUE}${BOLD}=============================================\n${NC}"
endef

all:
	$(call print_header,Building in all directories)
	@pass=0; fail=0; \
	for dir in $(DIRS); do \
		printf "${YELLOW}Building in $$dir...${NC}\n"; \
		if [ "$$dir" = "test10" ]; then \
			printf "[${GREEN}PASS${NC}] No build needed in $$dir\n"; \
			pass=$$((pass+1)); \
			printf "  To run test10: cd test10 && ./test10_script\n"; \
		elif $(MAKE) -C $$dir >/dev/null 2>&1; then \
			printf "[${GREEN}PASS${NC}] Build successful in $$dir\n"; \
			pass=$$((pass+1)); \
			printf "  To run $$dir: cd $$dir && ./$$dir\n"; \
		else \
			printf "[${RED}FAIL${NC}] Build failed in $$dir\n"; \
			fail=$$((fail+1)); \
		fi; \
	done; \
	printf "${BLUE}${BOLD}=============================================\n"; \
	printf "${BLUE}${BOLD}  Summary: ${GREEN}%d passed${NC}${BLUE}${BOLD}, ${RED}%d failed${NC}\n" $$pass $$fail; \
	printf "${BLUE}${BOLD}=============================================\n${NC}"; \
	if [ $$fail -ne 0 ]; then exit 1; fi

test:
	$(call print_header,Running all tests)
	@mkdir -p output
	@pass=0; fail=0; \
	for dir in $(DIRS); do \
		printf "${YELLOW}Running test in $$dir...${NC}\n"; \
		if [ "$$dir" = "test10" ]; then \
			printf "[${GREEN}PASS${NC}] Test successful in $$dir (no build needed)\n"; \
			pass=$$((pass+1)); \
			printf "  To run test10: cd test10 && ./test10_script\n"; \
		elif $(MAKE) -C $$dir test_$$(echo $$dir | sed 's/test//') >output/$$dir.log 2>&1; then \
			printf "[${GREEN}PASS${NC}] Test successful in $$dir\n"; \
			pass=$$((pass+1)); \
			printf "  To run $$dir: cd $$dir && ./$$dir\n"; \
		else \
			printf "[${RED}FAIL${NC}] Test failed in $$dir\n"; \
			fail=$$((fail+1)); \
		fi; \
	done; \
	printf "${BLUE}${BOLD}=============================================\n"; \
	printf "${BLUE}${BOLD}  Summary: ${GREEN}%d passed${NC}${BLUE}${BOLD}, ${RED}%d failed${NC}\n" $$pass $$fail; \
	printf "${BLUE}${BOLD}  Logs: ${YELLOW}output/*.log${NC}\n"; \
	printf "${BLUE}${BOLD}=============================================\n${NC}"; \
	if [ $$fail -ne 0 ]; then exit 1; fi

define run_test
	@printf "${YELLOW}Running $(1) test in $$dir...${NC}\n"
	@if $(MAKE) -C $$dir test_$(1) >/dev/null 2>&1; then \
		printf "[${GREEN}PASS${NC}] $(1) test successful in $$dir\n"; \
		pass=$$((pass+1)); \
	else \
		printf "[${RED}FAIL${NC}] $(1) test failed in $$dir\n"; \
		fail=$$((fail+1)); \
	fi
endef

clean:
	$(call print_header,Cleaning Up)
	@pass=0; fail=0; \
	for dir in $(DIRS); do \
		printf "${YELLOW}Cleaning in $$dir...${NC}\n"; \
		if [ "$$dir" = "test10" ]; then \
			printf "[${GREEN}PASS${NC}] No cleanup needed in $$dir\n"; \
			pass=$$((pass+1)); \
		elif $(MAKE) -C $$dir clean >/dev/null 2>&1; then \
			printf "[${GREEN}PASS${NC}] Cleanup successful in $$dir\n"; \
			pass=$$((pass+1)); \
		else \
			printf "[${RED}FAIL${NC}] Cleanup failed in $$dir\n"; \
			fail=$$((fail+1)); \
		fi; \
	done; \
	printf "${BLUE}${BOLD}=============================================\n"; \
	printf "${BLUE}${BOLD}  Summary: ${GREEN}%d passed${NC}${BLUE}${BOLD}, ${RED}%d failed${NC}\n" $$pass $$fail; \
	printf "${BLUE}${BOLD}=============================================\n${NC}"; \
	if [ $$fail -ne 0 ]; then exit 1; fi

test_1:
	@echo "\n=== Running Test 1 ==="
	@cd test1 && ./test1

test_2:
	@echo "\n=== Running Test 2 ==="
	@cd test2 && ./test2

test_3:
	@echo "\n=== Running Test 3 ==="
	@cd test3 && ./test3

test_4:
	@echo "\n=== Running Test 4 ==="
	@cd test4 && ./test4

test_5:
	@echo "\n=== Running Test 5 ==="
	@cd test5 && ./test5

test_6:
	@echo "\n=== Running Test 6 ==="
	@cd test6 && ./test6

test_7:
	@echo "\n=== Running Test 7 ==="
	@cd test7 && ./test7

test_8:
	@echo "\n=== Running Test 8 ==="
	@cd test8 && ./test8

test_9:
	@echo "\n=== Running Test 9 ==="
	@cd test9 && ./test9

test_10:
	@echo "\n=== Running Test 10 ==="
	@echo "To run test 10, simply run:"
	@echo "cd test10 && bash sysbench.sh"
	@echo "(This will use ./sysbench in the same directory)"

help:
	@echo "\n${BOLD}Available targets:${NC}"
	@echo "  make all      - Build all tests"
	@echo "  make test     - Run all tests"
	@echo "  make clean    - Clean all tests"
	@echo "  make test_1   - Run test1 individually (cd test1 && ./test1)"
	@echo "  make test_2   - Run test2 individually (cd test2 && ./test2)"
	@echo "  make test_3   - Run test3 individually (cd test3 && ./test3)"
	@echo "  make test_4   - Run test4 individually (cd test4 && ./test4)"
	@echo "  make test_5   - Run test5 individually (cd test5 && ./test5)"
	@echo "  make test_6   - Run test6 individually (cd test6 && ./test6)"
	@echo "  make test_7   - Run test7 individually (cd test7 && ./test7)"
	@echo "  make test_8   - Run test8 individually (cd test8 && ./test8)"
	@echo "  make test_9   - Run test9 individually (cd test9 && ./test9)"
	@echo "  make test_10  - Run test10 individually (cd test10 && bash sysbench.sh)"
	@echo "                 (Just run sysbench.sh, nothing else is needed)"
	@echo "                 (This will use ./sysbench in the same directory)"
	@echo "\nFor more details, see the Makefile."