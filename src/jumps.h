#ifndef JUMPS_H
#define JUMPS_H

#define   TRY__                 goto try__; try__: { // int error__ = 0;
#define   CHECK__(cond, err)    { if (cond)  { error__ = err; goto catch__; } }
#define   ERROR__               (error__)
#define   CATCH__               goto finally__; catch__:
#define   RETRY__               goto try__;
#define   FINALLY__             finally__:
#define   RETURN__              goto finally__;
#define   FAIL__                goto catch__;
#define   ENDTRY__              }

#endif // JUMPS_H
