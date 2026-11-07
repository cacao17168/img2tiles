#include "test_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifndef _WIN32
#include <unistd.h>
#include <sys/wait.h>
#endif

#define ERR_IMG_UNK_ARG    1
#define ERR_IMG_NOFILE     2
#define ERR_IMG_VQ         3
#define ERR_IMG_NOPERM     4
#define ERR_IMG_PROC       5
#define ERR_IMG_NOCOL      6
#define ERR_IMG_MEMERR     7
#define ERR_IMG_HEX        8

typedef struct {
    unsigned char r, g, b, a;
} color_t;

typedef struct {
    int x, y;
} pixel_t;

typedef struct {
    int x, y;
    int w, h;
} rect_t;

typedef struct {
    rect_t *rects;
    size_t count, cap;
} rect_list_t;

color_t _img2tiles_hex_to_rgb(char* hex_code);
rect_list_t _img2tiles_merge_pixels(pixel_t *pixels, size_t pixel_count, int img_width, int img_height);
char *_img2tiles_add_extension(char *src, char *ext);
bool _img2tiles_color_matches(color_t *src, color_t *trgt, int tol);
int img2tiles_main(int argc, char **argv);

CT_TEST(test_hex_to_rgb_valid) {
    color_t c;
    char hex1[] = "#FF0000FF";
    c = _img2tiles_hex_to_rgb(hex1);
    CT_ASSERT_EQ(255, c.r);
    CT_ASSERT_EQ(0,   c.g);
    CT_ASSERT_EQ(0,   c.b);
    CT_ASSERT_EQ(255, c.a);

    char hex2[] = "00FF00FF";
    c = _img2tiles_hex_to_rgb(hex2);
    CT_ASSERT_EQ(0,   c.r);
    CT_ASSERT_EQ(255, c.g);
    CT_ASSERT_EQ(0,   c.b);
    CT_ASSERT_EQ(255, c.a);

    char hex3[] = "#12345678";
    c = _img2tiles_hex_to_rgb(hex3);
    CT_ASSERT_EQ(0x12, c.r);
    CT_ASSERT_EQ(0x34, c.g);
    CT_ASSERT_EQ(0x56, c.b);
    CT_ASSERT_EQ(0x78, c.a);

    char hex4[] = "#aabbccdd";
    c = _img2tiles_hex_to_rgb(hex4);
    CT_ASSERT_EQ(0xAA, c.r);
    CT_ASSERT_EQ(0xBB, c.g);
    CT_ASSERT_EQ(0xCC, c.b);
    CT_ASSERT_EQ(0xDD, c.a);

    return 0;
}

#ifndef _WIN32
CT_TEST(test_hex_to_rgb_invalid_length) {
    pid_t pid = fork();
    if (pid == 0) {
        char hex[] = "#FFF";
        _img2tiles_hex_to_rgb(hex);
        exit(0);
    }
    int status;
    waitpid(pid, &status, 0);
    CT_ASSERT_TRUE(WIFEXITED(status));
    CT_ASSERT_EQ(ERR_IMG_HEX, WEXITSTATUS(status));
    return 0;
}
#endif

CT_TEST(test_merge_pixels_empty) {
    rect_list_t r = _img2tiles_merge_pixels(NULL, 0, 10, 10);
    CT_ASSERT_EQ(0, r.count);
    CT_ASSERT_TRUE(r.rects == NULL);
    free(r.rects);
    return 0;
}

CT_TEST(test_merge_pixels_single) {
    pixel_t px = {2, 3};
    rect_list_t r = _img2tiles_merge_pixels(&px, 1, 10, 10);
    CT_ASSERT_EQ(1, r.count);
    CT_ASSERT_EQ(2, r.rects[0].x);
    CT_ASSERT_EQ(3, r.rects[0].y);
    CT_ASSERT_EQ(1, r.rects[0].w);
    CT_ASSERT_EQ(1, r.rects[0].h);
    free(r.rects);
    return 0;
}

CT_TEST(test_merge_pixels_horizontal) {
    pixel_t px[] = {{0,0},{1,0},{2,0}};
    rect_list_t r = _img2tiles_merge_pixels(px, 3, 10, 10);
    CT_ASSERT_EQ(1, r.count);
    CT_ASSERT_EQ(0, r.rects[0].x);
    CT_ASSERT_EQ(0, r.rects[0].y);
    CT_ASSERT_EQ(3, r.rects[0].w);
    CT_ASSERT_EQ(1, r.rects[0].h);
    free(r.rects);
    return 0;
}

CT_TEST(test_merge_pixels_vertical) {
    pixel_t px[] = {{0,0},{0,1},{0,2}};
    rect_list_t r = _img2tiles_merge_pixels(px, 3, 10, 10);
    CT_ASSERT_EQ(1, r.count);
    CT_ASSERT_EQ(0, r.rects[0].x);
    CT_ASSERT_EQ(0, r.rects[0].y);
    CT_ASSERT_EQ(1, r.rects[0].w);
    CT_ASSERT_EQ(3, r.rects[0].h);
    free(r.rects);
    return 0;
}

CT_TEST(test_merge_pixels_rectangle) {
    pixel_t px[] = {{1,1},{2,1},{1,2},{2,2}};
    rect_list_t r = _img2tiles_merge_pixels(px, 4, 10, 10);
    CT_ASSERT_EQ(1, r.count);
    CT_ASSERT_EQ(1, r.rects[0].x);
    CT_ASSERT_EQ(1, r.rects[0].y);
    CT_ASSERT_EQ(2, r.rects[0].w);
    CT_ASSERT_EQ(2, r.rects[0].h);
    free(r.rects);
    return 0;
}

CT_TEST(test_merge_pixels_disjoint) {
    pixel_t px[] = {{0,0},{5,5}};
    rect_list_t r = _img2tiles_merge_pixels(px, 2, 10, 10);
    CT_ASSERT_EQ(2, r.count);
    int found00 = 0, found55 = 0;
    for (size_t i = 0; i < r.count; i++) {
        if (r.rects[i].x == 0 && r.rects[i].y == 0 &&
            r.rects[i].w == 1 && r.rects[i].h == 1) found00 = 1;
        if (r.rects[i].x == 5 && r.rects[i].y == 5 &&
            r.rects[i].w == 1 && r.rects[i].h == 1) found55 = 1;
    }
    CT_ASSERT_TRUE(found00);
    CT_ASSERT_TRUE(found55);
    free(r.rects);
    return 0;
}

CT_TEST(test_merge_pixels_l_shape) {
    pixel_t px[] = {{0,0},{1,0},{0,1}};
    rect_list_t r = _img2tiles_merge_pixels(px, 3, 10, 10);
    CT_ASSERT_EQ(2, r.count);
    CT_ASSERT_EQ(0, r.rects[0].x);
    CT_ASSERT_EQ(0, r.rects[0].y);
    CT_ASSERT_EQ(2, r.rects[0].w);
    CT_ASSERT_EQ(1, r.rects[0].h);
    CT_ASSERT_EQ(0, r.rects[1].x);
    CT_ASSERT_EQ(1, r.rects[1].y);
    CT_ASSERT_EQ(1, r.rects[1].w);
    CT_ASSERT_EQ(1, r.rects[1].h);
    free(r.rects);
    return 0;
}

CT_TEST(test_add_extension) {
    char *res;

    char src1[] = "file.png";
    res = _img2tiles_add_extension(src1, ".json");
    CT_ASSERT_TRUE(res != NULL);
    CT_ASSERT_EQ(0, strcmp(res, "file.json"));
    free(res);

    char src2[] = "file";
    res = _img2tiles_add_extension(src2, ".json");
    CT_ASSERT_TRUE(res != NULL);
    CT_ASSERT_EQ(0, strcmp(res, "file.json"));
    free(res);

    char src3[] = "file.tar.gz";
    res = _img2tiles_add_extension(src3, ".json");
    CT_ASSERT_TRUE(res != NULL);
    CT_ASSERT_EQ(0, strcmp(res, "file.tar.json"));
    free(res);

    char src4[] = "file.";
    res = _img2tiles_add_extension(src4, ".json");
    CT_ASSERT_TRUE(res != NULL);
    CT_ASSERT_EQ(0, strcmp(res, "file.json"));
    free(res);

    char src5[] = "file";
    res = _img2tiles_add_extension(src5, "json");
    CT_ASSERT_TRUE(res != NULL);
    CT_ASSERT_EQ(0, strcmp(res, "filejson"));
    free(res);

    return 0;
}


CT_TEST(test_color_matches_exact) {
    color_t a = {100, 150, 200, 255};
    color_t b = {100, 150, 200, 255};
    CT_ASSERT_TRUE(_img2tiles_color_matches(&a, &b, 0));
    CT_ASSERT_TRUE(_img2tiles_color_matches(&a, &b, 10));
    return 0;
}

CT_TEST(test_color_matches_within_tolerance) {
    color_t a = {100, 100, 100, 255};
    color_t b = {105,  95, 100, 255};
    CT_ASSERT_TRUE(_img2tiles_color_matches(&a, &b, 5));
    CT_ASSERT_TRUE(_img2tiles_color_matches(&a, &b, 10));
    CT_ASSERT_FALSE(_img2tiles_color_matches(&a, &b, 4));
    return 0;
}

CT_TEST(test_color_matches_alpha) {
    color_t a = {100, 100, 100, 255};
    color_t b = {100, 100, 100, 250};
    CT_ASSERT_TRUE(_img2tiles_color_matches(&a, &b, 5));
    CT_ASSERT_FALSE(_img2tiles_color_matches(&a, &b, 4));
    return 0;
}

CT_TEST(test_color_matches_different) {
    color_t a = {100, 100, 100, 255};
    color_t b = {200, 100, 100, 255};
    CT_ASSERT_FALSE(_img2tiles_color_matches(&a, &b, 10));
    return 0;
}

CT_TEST(test_main_help) {
    char *argv[] = {"img2tiles", "--help", NULL};
    int ret = img2tiles_main(2, argv);
    CT_ASSERT_EQ(0, ret);
    return 0;
}

CT_TEST(test_main_no_colors) {
    char *argv[] = {"img2tiles", NULL};
    int ret = img2tiles_main(1, argv);
    CT_ASSERT_EQ(ERR_IMG_NOCOL, ret);
    return 0;
}

CT_TEST(test_main_unknown_arg) {
    char *argv[] = {"img2tiles", "--unknown", NULL};
    int ret = img2tiles_main(2, argv);
    CT_ASSERT_EQ(ERR_IMG_UNK_ARG, ret);
    return 0;
}

#ifndef _WIN32
CT_TEST(test_main_missing_file) {
    pid_t pid = fork();
    if (pid == 0) {
        char *argv[] = {"img2tiles", "-f", "nonexistent.png", "FF0000FF", NULL};
        img2tiles_main(4, argv);
        exit(0);
    }
    int status;
    waitpid(pid, &status, 0);
    CT_ASSERT_TRUE(WIFEXITED(status));
    CT_ASSERT_EQ(ERR_IMG_PROC, WEXITSTATUS(status));
    return 0;
}

CT_TEST(test_main_verbose_quiet) {
    pid_t pid = fork();
    if (pid == 0) {
        char *argv[] = {"img2tiles", "-v", "-q", "-f", "dummy.png",
                        "FF0000FF", NULL};
        img2tiles_main(6, argv);
        exit(0);
    }
    int status;
    waitpid(pid, &status, 0);
    CT_ASSERT_TRUE(WIFEXITED(status));
    CT_ASSERT_EQ(ERR_IMG_VQ, WEXITSTATUS(status));
    return 0;
}
#endif

static int create_ppm(const char *filename, int w, int h,
                      const unsigned char *rgb_data) {
    FILE *f = fopen(filename, "wb");
    if (!f) return -1;
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    fwrite(rgb_data, 1, w * h * 3, f);
    fclose(f);
    return 0;
}

CT_TEST(test_main_success) {
    const char *ppm_file  = "test_img.ppm";
    const char *json_file = "test_img.json";

    unsigned char pixels[2*2*3] = {
        255, 0, 0,
        0, 255, 0,
        255, 0, 0,
        0, 0, 255
    };
    if (create_ppm(ppm_file, 2, 2, pixels) != 0) {
        ct_fail_record("Failed to create test PPM");
    }

    char *argv[] = {"img2tiles", "-f", (char*)ppm_file, "FF0000FF", NULL};
    int ret = img2tiles_main(4, argv);
    CT_ASSERT_EQ(0, ret);

    FILE *f = fopen(json_file, "r");
    CT_ASSERT_TRUE(f != NULL);
    if (f) {
        char buf[4096];
        size_t n = fread(buf, 1, sizeof(buf)-1, f);
        buf[n] = '\0';
        fclose(f);

        CT_ASSERT_TRUE(strstr(buf, "\"x\": 0")   != NULL);
        CT_ASSERT_TRUE(strstr(buf, "\"y\": 0")   != NULL);
        CT_ASSERT_TRUE(strstr(buf, "\"w\": 1")   != NULL);
        CT_ASSERT_TRUE(strstr(buf, "\"h\": 2")   != NULL);
        CT_ASSERT_TRUE(strstr(buf, "\"r\": 255") != NULL);
        CT_ASSERT_TRUE(strstr(buf, "\"g\": 0")   != NULL);
        CT_ASSERT_TRUE(strstr(buf, "\"b\": 0")   != NULL);
        CT_ASSERT_TRUE(strstr(buf, "\"a\": 255") != NULL);
    }

    remove(ppm_file);
    remove(json_file);
    return 0;
}
