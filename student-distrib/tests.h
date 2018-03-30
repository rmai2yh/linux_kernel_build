#ifndef TESTS_H
#define TESTS_H

/* TEST FLAGS FOR CHEKCPOINT 1 */
#define TERMINAL_READ_TEST_FLAG 0
#define TERMINAL_WRITE_TEST_FLAG 0
#define RTC_TEST_FLAG 0
#define PAGE_FAULT_EXCEPTION_TEST_FLAG 0
#define VIDEO_PAGING_TEST_FLAG 0
#define DEFAULT_FS_TEST_FLAG 0
#define FS_LS_TEST_FLAG 0
#define FS_PRINT_BY_NAME_TEST_FLAG 0
#define FS_PRINT_BY_INDEX_TEST_FLAG 0
#define SYSCALL_TEST_FLAG 1

// test launcher
void launch_tests();
void exception_test();
#endif /* TESTS_H */
