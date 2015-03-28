/**
 * @file test_decaf.cxx
 * @author Mike Hamburg
 *
 * @copyright
 *   Copyright (c) 2015 Cryptography Research, Inc.  \n
 *   Released under the MIT License.  See LICENSE.txt for license information.
 *
 * @brief C++ tests, because that's easier.
 */

#include "decaf.hxx"
#include "shake.h"
#include <stdio.h>

typedef decaf::decaf<448>::Scalar Scalar;
typedef decaf::decaf<448>::Point Point;
typedef decaf::decaf<448>::Precomputed Precomputed;

static const long NTESTS = 10000;

static void print(const char *name, const Scalar &x) {
    unsigned char buffer[DECAF_448_SCALAR_BYTES];
    x.encode(buffer);
    printf("  %s = 0x", name);
    for (int i=sizeof(buffer)-1; i>=0; i--) {
        printf("%02x", buffer[i]);
    }
    printf("\n");
}

static void print(const char *name, const Point &x) {
    unsigned char buffer[DECAF_448_SER_BYTES];
    x.encode(buffer);
    printf("  %s = 0x", name);
    for (int i=sizeof(buffer)-1; i>=0; i--) {
        printf("%02x", buffer[i]);
    }
    printf("\n");
}

static bool passing = true;

class Test {
public:
    bool passing_now;
    Test(const char *test) {
        passing_now = true;
        printf("%s...", test);
        if (strlen(test) < 27) printf("%*s",int(27-strlen(test)),"");
        fflush(stdout);
    }
    ~Test() {
        if (std::uncaught_exception()) {
            fail();
            printf("  due to uncaught exception.\n");
        }
        if (passing_now) printf("[PASS]\n");
    }
    void fail() {
        if (!passing_now) return;
        passing_now = passing = false;
        printf("[FAIL]\n");
    }
};

static bool arith_check(
    Test &test,
    const Scalar &x,
    const Scalar &y,
    const Scalar &z,
    const Scalar &r,
    const Scalar &l,
    const char *name
) {
    if (l == r) return true;
    test.fail();
    printf("  %s", name);
    print("x", x);
    print("y", y);
    print("z", z);
    print("lhs", r);
    print("rhs", l);
    return false;
}

static bool point_check(
    Test &test,
    const Point &p,
    const Point &q,
    const Point &R,
    const Scalar &x,
    const Scalar &y,
    const Point &r,
    const Point &l,
    const char *name
) {
    if (l == r) return true;
    test.fail();
    printf("  %s", name);
    print("x", x);
    print("y", y);
    print("p", p);
    print("q", q);
    print("r", R);
    print("lhs", r);
    print("rhs", l);
    return false;
}

static void test_arithmetic() {
    keccak_sponge_t sponge;
    unsigned char buffer[DECAF_448_SCALAR_BYTES+8];
    spongerng_init_from_buffer(sponge, (const uint8_t *)"test_arithmetic", 16, 1);
    
    Test test("Arithmetic");
    Scalar x(0),y(0),z(0);
    arith_check(test,x,y,z,INT_MAX,(decaf_word_t)INT_MAX,"cast from max");
    arith_check(test,x,y,z,INT_MIN,-Scalar(1+(decaf_word_t)INT_MAX),"cast from min");
        
    for (int i=0; i<NTESTS*10 && test.passing_now; i++) {
        /* TODO: pathological cases */
        size_t sob = sizeof(buffer) - (i%16);
        spongerng_next(sponge, buffer, sob);
        Scalar x(buffer, sob);
        spongerng_next(sponge, buffer, sob);
        Scalar y(buffer, sob);
        spongerng_next(sponge, buffer, sob);
        Scalar z(buffer, sob);
        

        arith_check(test,x,y,z,x+y,y+x,"commute add");
        arith_check(test,x,y,z,x,x+0,"ident add");
        arith_check(test,x,y,z,x,x-0,"ident sub");
        arith_check(test,x,y,z,x+(y+z),(x+y)+z,"assoc add");
        arith_check(test,x,y,z,x*(y+z),x*y + x*z,"distributive mul/add");
        arith_check(test,x,y,z,x*(y-z),x*y - x*z,"distributive mul/add");
        arith_check(test,x,y,z,x*(y*z),(x*y)*z,"assoc mul");
        arith_check(test,x,y,z,x*y,y*x,"commute mul");
        arith_check(test,x,y,z,x,x*1,"ident mul");
        arith_check(test,x,y,z,0,x*0,"mul by 0");
        arith_check(test,x,y,z,-x,x*-1,"mul by -1");
        arith_check(test,x,y,z,x+x,x*2,"mul by 2");
        
        if (i%20) continue;
        if (y!=0) arith_check(test,x,y,z,x*y/y,x,"invert");
        arith_check(test,x,y,z,x/0,0,"invert0");
    }
}


static void test_ec() {
    keccak_sponge_t sponge;
    unsigned char buffer[2*DECAF_448_SCALAR_BYTES];
    spongerng_init_from_buffer(sponge, (const uint8_t *)"test_ec", 8, 1);
    
    Test test("EC");
    

    Point id = Point::identity(), base = Point::base();
    point_check(test,id,id,id,0,0,Point::from_hash(std::string("")),id,"fh0");
    point_check(test,id,id,id,0,0,Point::from_hash(std::string("\x01")),id,"fh1");
    
    for (int i=0; i<NTESTS && test.passing_now; i++) {
        /* TODO: pathological cases */
        size_t sob = sizeof(buffer);
        spongerng_next(sponge, buffer, sob);
        Scalar x(buffer, sob);
        spongerng_next(sponge, buffer, sob);
        Scalar y(buffer, sob);
        spongerng_next(sponge, buffer, sob);
        Point p = Point::from_hash(buffer);
        spongerng_next(sponge, buffer, sob);
        Point q = Point::from_hash(buffer);
        spongerng_next(sponge, buffer, sob);
        Point r = Point::from_hash(buffer);
        
        point_check(test,p,q,r,0,0,p,Point((std::string)p),"round-trip");
        point_check(test,p,q,r,0,0,p+q,q+p,"commute add");
        point_check(test,p,q,r,0,0,p+(q+r),(p+q)+r,"assoc add");
        
        if (i%10) continue;
        point_check(test,p,q,r,x,0,x*(p+q),x*p+x*q,"distr mul");
        point_check(test,p,q,r,x,y,(x*y)*p,x*(y*p),"assoc mul");
        point_check(test,p,q,r,x,y,x*p+y*q,Point::double_scalarmul(x,p,y,q),"ds mul");
        point_check(test,base,q,r,x,y,x*base+y*q,q.non_secret_combo_with_base(y,x),"ds vt mul");
        point_check(test,p,q,r,x,0,Precomputed(p)*x,p*x,"precomp mul");
        point_check(test,p,q,r,0,0,r,
            Point::from_hash_nonuniform(buffer)
            +Point::from_hash_nonuniform(&buffer[DECAF_448_SCALAR_BYTES]),
            "unih = hash+add"
        );
        
        // TODO: test hash_u(x+x) == hash_nu(x) + hash_nu(x)???
    }
}

int main(int argc, char **argv) {
    (void) argc; (void) argv;
    
    test_arithmetic();
    test_ec();
    
    if (passing) printf("Passed all tests.\n");
    
    return passing ? 0 : 1;
}
