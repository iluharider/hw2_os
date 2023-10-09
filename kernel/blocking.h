int blocking_get();

int blocking_block(int);

int blocking_release(int);

int blocking_delete(int);

enum blocking_cmd {BL_GET, BL_ACQUIRE, BL_RELEASE, BL_DELETE};