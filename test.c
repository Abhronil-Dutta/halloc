#include "halloc.h"

#include <stdint.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;

static void log_result(const char* label, int pass) {
	tests_run++;
	if (pass) {
		tests_passed++;
	}
	printf("[%-4s] %s\n", pass ? "PASS" : "FAIL", label);
}

static void print_op_header(const char* title) {
	printf("\n============================================================\n");
	printf("%s\n", title);
	printf("============================================================\n");
}

static void print_alloc(const char* label, size_t size, void* ptr) {
	printf("ALLOC %-26s size=%-5zu -> %p\n", label, size, ptr);
}

static void print_free(const char* label, void* ptr, int status) {
	printf("FREE  %-26s addr=%p -> status=%d\n", label, ptr, status);
}

static int ptr_equal(void* a, void* b) {
	return (uintptr_t)a == (uintptr_t)b;
}

static void test_basic_allocation_and_free(void) {
	print_op_header("1) Basic Allocation and Freeing");

	size_t sizes[] = {8, 16, 32, 256, 512, 1024, 2048};
	const char* labels[] = {
		"small-8", "small-16", "small-32",
		"medium-256", "medium-512",
		"large-1024", "large-2048"
	};
	void* ptrs[7] = {0};

	for (int i = 0; i < 7; i++) {
		ptrs[i] = halloc(sizes[i]);
		print_alloc(labels[i], sizes[i], ptrs[i]);
		log_result("allocation returned non-NULL", ptrs[i] != NULL);

		if (ptrs[i] != NULL) {
			memset(ptrs[i], 0xA0 + i, sizes[i]);
		}
	}

	for (int i = 6; i >= 0; i--) {
		int st = hfree(ptrs[i]);
		print_free(labels[i], ptrs[i], st);
		log_result("free returned success", st == 1);
		printf("After free of %s, address was %p\n", labels[i], ptrs[i]);
	}
}

static void test_edge_cases(void) {
	print_op_header("2) Edge Cases");

	void* z = halloc(0);
	print_alloc("zero-size", 0, z);
	log_result("zero-size allocation handled (no crash)", 1);
	if (z != NULL) {
		int stz = hfree(z);
		print_free("zero-size", z, stz);
		log_result("free zero-size result is valid", stz == 0 || stz == 1);
	}

	int st_null = hfree(NULL);
	print_free("NULL", NULL, st_null);
	log_result("free(NULL) returns failure", st_null == 0);

	void* p = halloc(64);
	print_alloc("double-free-target", 64, p);
	log_result("double-free target alloc non-NULL", p != NULL);

	if (p != NULL) {
		int first = hfree(p);
		print_free("double-free first", p, first);
		log_result("first free succeeds", first == 1);

		int second = hfree(p);
		print_free("double-free second", p, second);
		log_result("second free fails", second == 0);
	}

	void* a = halloc(80);
	print_alloc("reuse-before-free", 80, a);
	log_result("reuse-before-free alloc non-NULL", a != NULL);

	if (a != NULL) {
		int fa = hfree(a);
		print_free("reuse-before-free", a, fa);
		log_result("reuse free succeeds", fa == 1);
	}

	void* b = halloc(48);
	print_alloc("reuse-after-free", 48, b);
	log_result("reuse-after-free alloc non-NULL", b != NULL);
	if (a != NULL && b != NULL) {
		printf("Reuse check: old=%p, new=%p\n", a, b);
	}
	if (b != NULL) {
		int fb = hfree(b);
		print_free("reuse-after-free", b, fb);
		log_result("reuse-after-free free succeeds", fb == 1);
	}
}

static void test_fragmentation_and_coalescing(void) {
	print_op_header("3) Fragmentation and Coalescing");

	printf("Scenario A: Allocate 3 blocks, free middle, fit new block in gap\n");
	void* a1 = halloc(128);
	void* a2 = halloc(128);
	void* a3 = halloc(128);
	print_alloc("A1", 128, a1);
	print_alloc("A2", 128, a2);
	print_alloc("A3", 128, a3);
	log_result("scenario A allocations non-NULL", a1 && a2 && a3);

	int fa2 = hfree(a2);
	print_free("A2", a2, fa2);
	log_result("scenario A middle free succeeds", fa2 == 1);

	void* a4 = halloc(96);
	print_alloc("A4 (should fit A2 gap)", 96, a4);
	log_result("scenario A re-allocation non-NULL", a4 != NULL);
	if (a4 != NULL && a2 != NULL) {
		int reused = ptr_equal(a4, a2);
		printf("Scenario A reuse check: old middle=%p, new=%p, reused=%s\n",
			   a2, a4, reused ? "YES" : "NO");
		log_result("scenario A reused middle gap", reused);
	}

	if (a1) print_free("A1", a1, hfree(a1));
	if (a3) print_free("A3", a3, hfree(a3));
	if (a4) print_free("A4", a4, hfree(a4));

	printf("\nScenario B: Allocate 4 blocks, free consecutive blocks, verify coalesce\n");
	void* b1 = halloc(120);
	void* b2 = halloc(120);
	void* b3 = halloc(120);
	void* b4 = halloc(120);
	print_alloc("B1", 120, b1);
	print_alloc("B2", 120, b2);
	print_alloc("B3", 120, b3);
	print_alloc("B4", 120, b4);
	log_result("scenario B allocations non-NULL", b1 && b2 && b3 && b4);

	int fb2 = hfree(b2);
	int fb3 = hfree(b3);
	print_free("B2", b2, fb2);
	print_free("B3", b3, fb3);
	log_result("scenario B frees succeed", fb2 == 1 && fb3 == 1);

	void* b5 = halloc(220);
	print_alloc("B5 (requires coalesced B2+B3)", 220, b5);
	log_result("scenario B large-in-gap alloc non-NULL", b5 != NULL);
	if (b5 != NULL && b2 != NULL) {
		int reused = ptr_equal(b5, b2);
		printf("Scenario B coalesce check: b2=%p, b5=%p, coalesced_reuse=%s\n",
			   b2, b5, reused ? "YES" : "NO");
		log_result("scenario B coalesced block reused", reused);
	}

	if (b1) print_free("B1", b1, hfree(b1));
	if (b4) print_free("B4", b4, hfree(b4));
	if (b5) print_free("B5", b5, hfree(b5));

	printf("\nScenario C: Allocate 5, free alternating, then allocate large block\n");
	void* c[5] = {0};
	for (int i = 0; i < 5; i++) {
		c[i] = halloc(100);
		char label[16];
		snprintf(label, sizeof(label), "C%d", i);
		print_alloc(label, 100, c[i]);
	}
	log_result("scenario C allocations non-NULL", c[0] && c[1] && c[2] && c[3] && c[4]);

	int f0 = hfree(c[0]);
	int f2 = hfree(c[2]);
	int f4 = hfree(c[4]);
	print_free("C0", c[0], f0);
	print_free("C2", c[2], f2);
	print_free("C4", c[4], f4);
	log_result("scenario C alternating frees succeed", f0 == 1 && f2 == 1 && f4 == 1);

	void* c_large = halloc(260);
	print_alloc("C-large-after-fragment", 260, c_large);
	log_result("scenario C large alloc non-NULL", c_large != NULL);
	if (c_large != NULL) {
		printf("Scenario C large alloc address: %p (fragmentation behavior visible by address)\n", c_large);
	}

	if (c[1]) print_free("C1", c[1], hfree(c[1]));
	if (c[3]) print_free("C3", c[3], hfree(c[3]));
	if (c_large) print_free("C-large", c_large, hfree(c_large));
}

static void test_heap_expansion(void) {
	print_op_header("4) Heap Expansion (sbrk Growth)");

	printf("Allocating blocks to exceed initial 1024-byte arena and force expansion\n");
	void* blocks[16] = {0};
	size_t block_sizes[16] = {
		300, 300, 300, 300,
		400, 400, 400, 400,
		512, 512, 600, 600,
		700, 700, 800, 800
	};

	for (int i = 0; i < 16; i++) {
		blocks[i] = halloc(block_sizes[i]);
		char label[24];
		snprintf(label, sizeof(label), "expand-%02d", i);
		print_alloc(label, block_sizes[i], blocks[i]);
		log_result("expansion alloc non-NULL", blocks[i] != NULL);
	}

	printf("Address progression (visual cue for heap growth):\n");
	for (int i = 0; i < 16; i++) {
		printf("  block[%02d] size=%-4zu addr=%p\n", i, block_sizes[i], blocks[i]);
	}

	for (int i = 15; i >= 0; i--) {
		if (blocks[i] != NULL) {
			int st = hfree(blocks[i]);
			char label[24];
			snprintf(label, sizeof(label), "expand-%02d", i);
			print_free(label, blocks[i], st);
			log_result("expansion free succeeds", st == 1);
		}
	}
}

static void test_memory_layout_and_reuse_summary(void) {
	print_op_header("5) Memory Layout, Address Tracking, Reuse Summary");

	void* p1 = halloc(64);
	void* p2 = halloc(64);
	void* p3 = halloc(64);
	print_alloc("L1", 64, p1);
	print_alloc("L2", 64, p2);
	print_alloc("L3", 64, p3);
	log_result("layout allocs non-NULL", p1 && p2 && p3);

	if (p2) {
		int st = hfree(p2);
		print_free("L2", p2, st);
		log_result("layout free L2 succeeds", st == 1);
	}

	void* p4 = halloc(48);
	print_alloc("L4", 48, p4);
	log_result("layout L4 alloc non-NULL", p4 != NULL);

	if (p2 != NULL && p4 != NULL) {
		int reused = ptr_equal(p2, p4);
		printf("Reuse summary: freed L2=%p, new L4=%p, reused=%s\n",
			   p2, p4, reused ? "YES" : "NO");
		log_result("layout expected reuse", reused);
	}

	if (p1) print_free("L1", p1, hfree(p1));
	if (p3) print_free("L3", p3, hfree(p3));
	if (p4) print_free("L4", p4, hfree(p4));
}

int main(void) {
	printf("halloc test harness start\n");
	printf("sizeof(Header) = %zu\n", sizeof(Header));
	printf("Expected API: halloc(size_t), hfree(void*)\n");

	test_basic_allocation_and_free();
	test_edge_cases();
	test_fragmentation_and_coalescing();
	test_heap_expansion();
	test_memory_layout_and_reuse_summary();

	printf("\n============================================================\n");
	printf("FINAL TEST SUMMARY: passed %d / %d\n", tests_passed, tests_run);
	printf("============================================================\n");

	return (tests_passed == tests_run) ? 0 : 1;
}
