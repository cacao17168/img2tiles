#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "third_party/yyjson/yyjson.h"
#include "third_party/ya_getopt/ya_getopt.h"

#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb/stb_image.h"

#define RED     "\033[31m"
#define RESET   "\033[0m"


#define IMG_SUCCESS        0
#define ERR_IMG_UNK_ARG    1
#define ERR_IMG_NOFILE     2
#define ERR_IMG_VQ         3
#define ERR_IMG_NOPERM     4
#define ERR_IMG_PROC       5
#define ERR_IMG_NOCOL      6
#define ERR_IMG_MEMERR     7
#define ERR_IMG_HEX        8

extern int verbose;

#define DEBUG(fmt, ...) \
    do { \
        if(verbose) { \
            printf("[DEBUG] " fmt "\n", ##__VA_ARGS__); \
        } \
    } while(0)

#define ERR_MSG(fmt, ...) \
    do { \
        fprintf(stderr, RED "[ERROR] " fmt RESET "\n", ##__VA_ARGS__); \
    } while (0)

typedef struct {
    unsigned char r, g, b, a;
} color_t;

typedef struct {
    int x, y;
} pixel_t;

typedef struct {
    color_t col;
    pixel_t *pixels;
    size_t count;
} tile_t;

typedef struct {
    int x, y;
    int w, h;
} rect_t;

typedef struct {
    rect_t *rects;
    size_t count;
} rect_list_t;



static void func_help(void) {
    printf("usage: img2tiles --file <file> [OPTIONS] <color 1> [color 2...]\n");
    printf(" \n");
    printf("--file        -f         point input file\n");
    printf("--help        -h         show this note\n");
    printf("--verbose     -v         debug info\n");
    printf("--quiet       -q         no output\n");
    printf("--tolerance   -t         set how strict is check(0 is the most strict)\n");
}

static void check_vq(int v, int q) {
    if(v && q) {
        ERR_MSG("can't use verbose and quiet at the same time.");
        exit(ERR_IMG_VQ);
    }
}

static unsigned char* load_image(char* filename, int* w, int *h, int *ch, int img_bytes) {
    unsigned char* raw = stbi_load(filename, w, h, ch, img_bytes);
    if(!raw) {
        ERR_MSG("file processing failed: %s", filename);
        exit(ERR_IMG_PROC);
    }
    return raw;
}

static color_t hex_to_rgb(char* hex_code) {
    color_t color = {0, 0, 0, 255};

    if (*hex_code == '#') hex_code++;

    if (strlen(hex_code) != 8) {
        ERR_MSG("hex must be 8 symbols");
        exit(ERR_IMG_HEX);
    };

    unsigned int r, g, b, a;

    if(sscanf(hex_code, "%02x%02x%02x%02x", &r, &g, &b, &a) == 4) {
        color.r = (unsigned char)r;
        color.g = (unsigned char)g;
        color.b = (unsigned char)b;
        color.a = (unsigned char)a;
    }
    return color;
}

static rect_list_t merge_pixels(pixel_t *pixels, int pixel_count, int img_width, int img_height) {
    rect_list_t result = {0};
    if(pixel_count <= 0) return result;

    bool* pixel_map = (bool*)calloc(img_width * img_height, sizeof(bool));
    if(!pixel_map) {
        ERR_MSG("pixel map memory allocating failed.");
        exit(ERR_IMG_MEMERR);
    }

    for(size_t i = 0; i < (size_t)pixel_count; i++) {
        int idx = pixels[i].y * img_width + pixels[i].x;
        pixel_map[idx] = true;
    }

    size_t startx = 0;
    size_t starty = 0;
    size_t endx = 0;
    size_t endy = 0;

    for(size_t i = 0; i < (size_t)pixel_count; i++) {
        startx = pixels[i].x;
        starty = pixels[i].y;
        size_t start_idx = starty * img_width + startx;

        if(!pixel_map[start_idx]) {
            continue;
        }

        endx = startx;
        endy = starty;

        while((endx + 1) < (size_t)img_width && pixel_map[starty * (size_t)img_width + (endx + 1)]) {
            endx++;
        }

        bool can_grow_down = true;
        while(can_grow_down) {
            size_t nexty = endy + 1;
            if(nexty >= (size_t)img_height) break;

            for(size_t x = startx; x <= endx; x++) {
                if(!pixel_map[nexty * img_width + x]) {
                    can_grow_down = false;
                    break;
                }
            }

            if(can_grow_down) {
                endy++;
            }
        }

        for(size_t y = starty; y <= endy; y++) {
            for(size_t x = startx; x <= endx; x++) {
                pixel_map[y * img_width + x] = false;
            }
        }

        rect_t *tmp = realloc(result.rects, sizeof(rect_t) * (result.count + 1));
        if(!tmp) {
            free(result.rects);
            free(pixel_map);
            ERR_MSG("rect memory allocating failed");
            exit(ERR_IMG_MEMERR);
        }

        result.rects = tmp;
        result.rects[result.count].x = startx;
        result.rects[result.count].y = starty;
        result.rects[result.count].w = endx - startx + 1;
        result.rects[result.count].h = endy - starty + 1;
        result.count++;
    }

    free(pixel_map);
    return result;
}

static char *add_extension(char *src, char *ext) {
    const char *dot = strrchr(src, '.');

    size_t len = dot ? (size_t)(dot - src) : strlen(src);
    size_t ext_len = strlen(ext);

    char *result = (char *)malloc(len + ext_len + 1);

    if (!result) return NULL;

    memcpy(result, src, len);
    strcpy(result + len, ext);

    return result;
}

static void lock_outputs(void) {
#if defined(_WIN32) || defined(_WIN64)
    freopen("nul", "w", stdout);
    freopen("nul", "w", stderr);
#else
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);
#endif
}

static bool color_matches(color_t *src, color_t *trgt, int tol) {
    return abs(src->r - trgt->r) <= tol &&
        abs(src->g - trgt->g) <= tol &&
        abs(src->b - trgt->b) <= tol &&
        abs(src->a - trgt->a) <= tol;
}

yyjson_mut_val *create_rect_json_obj(yyjson_mut_doc *rdoc, rect_t *rct, color_t *clr) {
    yyjson_mut_val *obj = yyjson_mut_obj(rdoc);

    yyjson_mut_obj_add_int(rdoc, obj, "x", rct->x);
    yyjson_mut_obj_add_int(rdoc, obj, "y", rct->y);
    yyjson_mut_obj_add_int(rdoc, obj, "w", rct->w);
    yyjson_mut_obj_add_int(rdoc, obj, "h", rct->h);

    yyjson_mut_val *color_obj = yyjson_mut_obj(rdoc);
    yyjson_mut_obj_add_int(rdoc, color_obj, "r", clr->r);
    yyjson_mut_obj_add_int(rdoc, color_obj, "g", clr->g);
    yyjson_mut_obj_add_int(rdoc, color_obj, "b", clr->b);
    yyjson_mut_obj_add_int(rdoc, color_obj, "a", clr->a);

    yyjson_mut_obj_add_val(rdoc, obj, "color", color_obj);
    return obj;
}



int main(int argc, char **argv) {
    int verbose = 0, quiet = 0;
    int width, height, channels;
    int opt;
    int tolerance = 0;
    char *input_file = NULL;

    static struct option args[] = {
        {"file",       required_argument, NULL, 'f'},
        {"help",       no_argument,       NULL, 'h'},
        {"verbose",    no_argument,       NULL, 'v'},
        {"quiet",      no_argument,       NULL, 'q'},
        {"tolerance",  required_argument, NULL, 't'},
        {0, 0, 0, 0}
    };

    while((opt = getopt_long(argc, argv, "f:hvqt:", args, NULL)) != -1) {
        switch(opt) {
            case 'f':
                input_file = optarg;
                break;
            case 'h':
                func_help();
                return 0;
            case 'v':
                verbose = 1;
                break;
            case 'q':
                quiet = 1;
                break;
            case 't':
                tolerance = strtol(optarg, NULL, 10);
                break;
            case '?':
                exit(ERR_IMG_UNK_ARG);
            default:
                abort();
        }
    }

    int col_count = argc - optind;
    tile_t tiles[col_count];
    memset(tiles, 0, sizeof(tiles));

    if(quiet) lock_outputs();

    if(optind < argc) {
        DEBUG("processing colors...");
        size_t tile_idx = 0;
        while(optind < argc) {
            tiles[tile_idx].col = hex_to_rgb(argv[optind]);
            tile_idx++;
            optind++;
        }
    } else {
        ERR_MSG("no colors received.");
        return ERR_IMG_NOCOL;
    }

    DEBUG("checking args for errors...");
    check_vq(verbose, quiet);

    DEBUG("opening file: %s", input_file);
    unsigned char* img = load_image(input_file, &width, &height, &channels, 4);
    DEBUG("file processed successfully");

    if(verbose) {
        printf("Image: %dx%d, channels: %d\n", width, height, channels);

        printf("Target color: R=%u G=%u B=%u A=%u\n",
            tiles[0].col.r,
            tiles[0].col.g,
            tiles[0].col.b,
            tiles[0].col.a
        );

        printf("\nFirst 10 pixels:\n");

        for (int i = 0; i < 10 && i < width * height; i++) {
            printf(
                "Pixel %d: R=%u G=%u B=%u A=%u\n",
                i,
                img[i * 4],
                img[i * 4 + 1],
                img[i * 4 + 2],
                img[i * 4 + 3]
            );
        }
    }

    for(size_t y = 0; y < (size_t)height; y++) {
        for(size_t x = 0; x < (size_t)width; x++) {
            size_t idx = (y * width + x) * 4;

            color_t src_col;
            src_col.r = img[idx + 0];
            src_col.g = img[idx + 1];
            src_col.b = img[idx + 2];
            src_col.a = img[idx + 3];

            for(size_t i = 0; i < (size_t)col_count; i++) {
                if(color_matches(&src_col, &tiles[i].col, tolerance)) {
                    tiles[i].count++;
                    pixel_t *tmp = realloc(tiles[i].pixels, sizeof(pixel_t) * tiles[i].count);
                    if (!tmp) {
                        ERR_MSG("pixel memory allocating failed.");
                        exit(ERR_IMG_MEMERR);
                    }

                    tiles[i].pixels = tmp;
                    tiles[i].pixels[tiles[i].count - 1].x = x;
                    tiles[i].pixels[tiles[i].count - 1].y = y;
                }
            }
        }
    }

    DEBUG("colors compare processed successfully.");

    DEBUG("creating json entry...");
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root_array = yyjson_mut_arr(doc);
    yyjson_mut_doc_set_root(doc, root_array);
    DEBUG("entry created successfully.");

    if(verbose) {
        for (int i = 0; i < col_count; i++) {
            printf(
                "Color #%02X%02X%02X: %zu pixels\n",
                tiles[i].col.r,
                tiles[i].col.g,
                tiles[i].col.b,
                tiles[i].count
            );
        }
    }

    DEBUG("adding tiles and colors to json...");
    for(size_t i = 0; i < (size_t)col_count; i++) {
        if(tiles[i].count == 0) continue;
        rect_list_t rects = merge_pixels(tiles[i].pixels, tiles[i].count, width, height);

        for(size_t r = 0; r < rects.count; r++) {
            yyjson_mut_val *rect_obj = create_rect_json_obj(doc, &rects.rects[r], &tiles[i].col);
            yyjson_mut_arr_add_val(root_array, rect_obj);
        }

        free(rects.rects);
    }
    DEBUG("added successfully");

    DEBUG("creating json file...");
    char *output_file = add_extension(input_file, ".json");
    yyjson_write_err err;
    bool success = yyjson_mut_write_file(output_file, doc, YYJSON_WRITE_PRETTY, NULL, &err);
    DEBUG("created: %s", output_file);

    if(success) {
        DEBUG("JSON file successfully generated!");
    } else {
        ERR_MSG("file write error: %s", err.msg);
    }

    free(output_file);
    yyjson_mut_doc_free(doc);
    stbi_image_free(img);
    for(size_t i = 0; i < (size_t)col_count; i++) {
        free(tiles[i].pixels);
    }

    DEBUG("resources freed.");
    return IMG_SUCCESS;
}
