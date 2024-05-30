#include <xinu.h>
#include <fs.h>

#ifdef FS

/**
 * TEST
 * MACRO to run a function and print whether it was successful or not
 */
#define TEST(function)                                         \
  if (nargs == 1 || strcmp(args[1], #function) == 0) {         \
    if ((function)() != SYSERR) {                              \
      printf("%-35s: [\033[32mPASS\033[39m]\n", #function);    \
    } else {                                                   \
      printf("%-35s: [\033[31mFAIL\033[39m]\n", #function);    \
    }                                                          \
  }


/* #define FSTEST_DEBUG */

/**
 * MACROs for unit testing
 */

/**
 * ASSERT_TEST
 * Run a function and report the return code
 */
#define ASSERT_TEST(function)                             printf("\033[33mTEST  %20s:%-3d %30s:\033[39m %d\n", __FILE__, __LINE__, #function, (function));

#ifdef FSTEST_DEBUG

#define ASSERT_TRUE(expr)     if (!(expr))              { printf("\033[31mERROR %20s:%-3d %30s()\033[39m '%s' != TRUE\n", __FILE__, __LINE__, __func__, #expr);       return SYSERR; }
#define ASSERT_PASS(function) if ((function) == SYSERR) { printf("\033[31mERROR %20s:%-3d %30s()\033[39m '%s' == SYSERR\n", __FILE__, __LINE__, __func__, #function); return SYSERR; }
#define ASSERT_FAIL(function) if ((function) != SYSERR) { printf("\033[31mERROR %20s:%-3d %30s()\033[39m '%s' != SYSERR\n", __FILE__, __LINE__, __func__, #function); return SYSERR; }

#else

/**
 * ASSERT_TRUE
 * Evaluate and expression and return SYSERR if false
 */
#define ASSERT_TRUE(expr)     if (!(expr))              { return SYSERR; }

/**
 * ASSERT_PASS
 * Run a function and return SYSERR if its return code is SYSERR
 */
#define ASSERT_PASS(function) if ((function) == SYSERR) { return SYSERR; }

/**
 * ASSERT_FAIL
 * Run a function and return SYSERR if its return code is not SYSERR
 */
#define ASSERT_FAIL(function) if ((function) != SYSERR) { return SYSERR; }

#endif


int fstest_testbitmask(void) {

  bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS);
  fs_mkfs(0, DEFAULT_NUM_INODES);

  fs_setmaskbit(31);
  fs_setmaskbit(95);
  fs_setmaskbit(159);
  fs_setmaskbit(223);
  fs_setmaskbit(287);
  fs_setmaskbit(351);
  fs_setmaskbit(415);
  fs_setmaskbit(479);
  fs_setmaskbit(90);
  fs_setmaskbit(154);
  fs_setmaskbit(218);
  fs_setmaskbit(282);
  fs_setmaskbit(346);
  fs_setmaskbit(347);
  fs_setmaskbit(348);
  fs_setmaskbit(349);
  fs_setmaskbit(350);
  fs_setmaskbit(100);
  fs_setmaskbit(164);
  fs_setmaskbit(228);
  fs_setmaskbit(292);
  fs_setmaskbit(356);
  fs_setmaskbit(355);
  fs_setmaskbit(354);
  fs_setmaskbit(353);
  fs_setmaskbit(352);

  fs_printfreemask();

  fs_clearmaskbit(31);
  fs_clearmaskbit(95);
  fs_clearmaskbit(159);
  fs_clearmaskbit(223);
  fs_clearmaskbit(287);
  fs_clearmaskbit(351);
  fs_clearmaskbit(415);
  fs_clearmaskbit(479);
  fs_clearmaskbit(90);
  fs_clearmaskbit(154);
  fs_clearmaskbit(218);
  fs_clearmaskbit(282);
  fs_clearmaskbit(346);
  fs_clearmaskbit(347);
  fs_clearmaskbit(348);
  fs_clearmaskbit(349);
  fs_clearmaskbit(350);
  fs_clearmaskbit(100);
  fs_clearmaskbit(164);
  fs_clearmaskbit(228);
  fs_clearmaskbit(292);
  fs_clearmaskbit(356);
  fs_clearmaskbit(355);
  fs_clearmaskbit(354);
  fs_clearmaskbit(353);
  fs_clearmaskbit(352);

  fs_printfreemask();

  fs_freefs(0);
  bs_freedev(0);

  return OK;
}

int fstest_seek() {
  int i = 0;

  for (i = 0; i < 10; i++) {
    ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
    ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))
    ASSERT_PASS(fs_create("test", O_CREAT))
    ASSERT_FAIL(fs_seek(0, 0))
    ASSERT_PASS(fs_freefs(0))
    ASSERT_PASS(bs_freedev(0))
  }

  return OK;
}

/**
 * Try to
 * - (re-)create and free the block device,
 * - (re-)create and free the file system, and
 * - create your first file "test"
 */
int fstest_mkdev() {

  int i, fd, fd2, wr_size, rd_size;
  char buf[2500] = "Flow control (data) From Wikipedia, the free encyclopedia Jump to navigationJump to search Not to be confused with Control flow. In data communications, flow control is the process of managing the rate of data transmission between two nodes to prevent a fast sender from overwhelming a slow receiver. It provides a mechanism for the receiver to control the transmission speed, so that the receiving node is not overwhelmed with data from transmitting node. Flow control should be distinguished from congestion control, which is used for controlling the flow of data when congestion has actually occurred.[1] Flow control mechanisms can be classified by whether or not the receiving node sends feedback to the sending node. Flow control is important because it is possible for a sending computer to transmit information at a faster rate than the destination computer can receive and process it. This can happen if the receiving computers have a heavy traffic load in comparison to the sending computer, or if the receiving computer has less processing power than the sending computer. Main article: Stop-and-wait ARQ Stop-and-wait flow control is the simplest form of flow control. In this method the message is broken into multiple frames, and the receiver indicates its readiness to receive a frame of data. The sender waits for a receipt acknowledgement (ACK) after every frame for a specified time (called a time out). The receiver sends the ACK to let the sender know that the frame of data was received correctly. The sender will then send the next frame only after the ACK. Operations Sender: Transmits a single frame at a time. Sender waits to receive ACK within time out. Receiver: Transmits acknowledgement (ACK) as it receives a frame. Go to step 1 when ACK is received, or time out is hit.";
  /*"Testing the filesystem !";*/
  char buf1[100] = {"\0"};
  char buf2[100] = "insert between";
  char buf3[40] = {"\0"};

  for (i = 0; i < 3; i++) {
    ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
    ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))
    ASSERT_TEST(fs_create("test", O_CREAT))
    ASSERT_TEST(fs_write(0, buf, strlen(buf)))
    ASSERT_TEST(fs_seek(0, 10))
    ASSERT_TEST(fs_read(0, buf1, strlen(buf)))
    ASSERT_TEST(fs_link("test", "test2"))
    ASSERT_TEST(fs_unlink("test"))
    ASSERT_TEST(fs_unlink("test2"))
    ASSERT_PASS(fs_freefs(0))
    ASSERT_PASS(bs_freedev(0))
  }

  bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS);
  fs_mkfs(0, DEFAULT_NUM_INODES);
  printf("----\n");
  fs_printfreemask();
  printf("----\n");

  fd = fs_create("test", O_CREAT);
  printf ("writing length: %d\n", strlen(buf));
  wr_size = fs_write(fd, buf, strlen(buf));
  printf("before seek__________\n");
  fs_print_oft();
  printf("\n");
  fs_seek(fd, 7);
  printf("after seek__________\n");
  fs_print_oft();
  printf("\n");
  printf ("reading length: 40\n");
  rd_size = fs_read(fd, buf1, 40);
  printf ("read length: %d\n", rd_size);
  printf("####### 1 Value read: %s ########\n", buf1);
  wr_size = fs_write(fd, buf2, strlen(buf2));
  printf("write size: %d\n", wr_size);
  fs_seek(fd, 0);
  rd_size = fs_read(fd, buf1, 100);
  printf ("read length: %d\n", rd_size);
  printf("####### 2 Value read: %s ########\n", buf1);

  fs_close(fd);

  fd2 = fs_open("test", O_RDONLY);
  
  rd_size = fs_read(fd2, buf3, 20);
  printf ("read length: %d\n", rd_size);
  printf("####### 3 Value read: %s ########\n", buf3);


  printf("root_dir before link_________\n");
  fs_print_dir();
  printf("__________\n");
  
  //fd = fs_open("test", O_RDWR);
  fs_link("test", "test1");
  fs_link("test1", "test2");
  fs_link("test", "test3");
  fs_create("newfile", O_CREAT);

  printf("root_dir after link_________\n");
  fs_print_dir();
  printf("__________\n");
  fs_print_oft();
  printf("\n");
  fs_print_inode(0);
  printf("\n");

  fs_unlink("test");
  fs_unlink("test1");

  printf("root_dir after unlink_________\n");
  fs_print_dir();
  printf("__________\n");
  fs_print_oft();
  printf("\n");
  fs_print_inode(0);
  printf("\n");
  
  printf("----\n");
  fs_printfreemask();
  printf("----\n");
  fs_freefs(0);
  bs_freedev(0);

  return OK;
}
#endif


int fstest(int nargs, char *args[]) {

  /* Output help, if '--help' argument was supplied */
  if (nargs == 2 && strncmp(args[1], "--help", 7) == 0) {
    printf("Usage: %s [TEST]\n\n", args[0]);
    printf("Description:\n");
    printf("\tFilesystem Test\n");
    printf("Options:\n");
    printf("\t--help\tdisplay this help and exit\n");
    return OK;
  }

  /* Check for correct number of arguments */
  if (nargs > 2) {
    fprintf(stderr, "%s: too many arguments\n", args[0]);
    fprintf(stderr, "Try '%s --help' for more information\n",
            args[0]);
    return SYSERR;
  }

#ifdef FS

  printf("\n\n\n");
  //TEST(fstest_testbitmask)
  
  /*printf("Before:_____________\n");
  //fs_printfreemask();
  printf("\n");
  //fs_print_dir();
  //printf("\n");
  fs_print_fsd();
  printf("\n");
  fs_print_oft();
  printf("\n");
  printf("____________________\n");
  */
  //TEST(fstest_mkdev)
  TEST(fstest_seek)
  
  /*printf("After:_____________\n");
  //fs_printfreemask();
  printf("\n");
  //fs_print_dir();
  //printf("\n");
  fs_print_fsd();
  printf("\n");
  fs_print_oft();
  printf("\n");
  fs_print_inode(0);
  printf("\n");
  printf("____________________\n");
  */
#else
  printf("No filesystem support\n");
#endif

  return OK;
}
