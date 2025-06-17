# Colors for output
GREEN := \033[0;32m
RED := \033[0;31m
YELLOW := \033[0;33m
BLUE := \033[0;34m
BOLD := \033[1m
NC := \033[0m # No Color

.PHONY: all clean test test_1 test_2 test_3 test_4 test_5 test_6 test_7 test_8

DIRS := test1 test2 test3 test4 test5 test6 test7 test8

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
		if $(MAKE) -C $$dir >/dev/null 2>&1; then \
			printf "[${GREEN}PASS${NC}] Build successful in $$dir\n"; \
			pass=$$((pass+1)); \
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
		if $(MAKE) -C $$dir test_$$(echo $$dir | sed 's/test//') >output/$$dir.log 2>&1; then \
			printf "[${GREEN}PASS${NC}] Test successful in $$dir\n"; \
			pass=$$((pass+1)); \
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
		if $(MAKE) -C $$dir clean >/dev/null 2>&1; then \
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
	@cd test7 && ./test7_program

test_8:
	@echo "\n=== Running Test 8 ==="
	@cd test8 && ./test8_program