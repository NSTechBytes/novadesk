/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#if defined(NOVADESK_HAS_QUICKJS)
#include "../../../../../third_party/quickjs/quickjs.h"
#endif

static char* nd_qjs_strdup(const char* s) {
    size_t n = s ? strlen(s) : 0;
    char* out = (char*)malloc(n + 1);
    if (!out) return NULL;
    if (n > 0) {
        memcpy(out, s, n);
    }
    out[n] = '\0';
    return out;
}

int nd_qjs_initialize(void** outRt, void** outCtx) {
#if !defined(NOVADESK_HAS_QUICKJS)
    if (outRt) *outRt = NULL;
    if (outCtx) *outCtx = NULL;
    return 0;
#else
    JSRuntime* rt;
    JSContext* ctx;

    if (!outRt || !outCtx) return 0;

    rt = JS_NewRuntime();
    if (!rt) return 0;

    ctx = JS_NewContext(rt);
    if (!ctx) {
        JS_FreeRuntime(rt);
        return 0;
    }

    *outRt = (void*)rt;
    *outCtx = (void*)ctx;
    return 1;
#endif
}

void nd_qjs_shutdown(void** rt, void** ctx) {
#if !defined(NOVADESK_HAS_QUICKJS)
    if (ctx) *ctx = NULL;
    if (rt) *rt = NULL;
#else
    if (ctx && *ctx) {
        JS_FreeContext((JSContext*)(*ctx));
        *ctx = NULL;
    }
    if (rt && *rt) {
        JS_FreeRuntime((JSRuntime*)(*rt));
        *rt = NULL;
    }
#endif
}

int nd_qjs_eval(void* ctx, const char* source, size_t len, const char* filename) {
#if !defined(NOVADESK_HAS_QUICKJS)
    (void)ctx;
    (void)source;
    (void)len;
    (void)filename;
    return 0;
#else
    JSContext* cctx;
    JSValue result;
    int ok;

    if (!ctx || !source || !filename) return 0;
    cctx = (JSContext*)ctx;
    result = JS_Eval(cctx, source, len, filename, JS_EVAL_TYPE_GLOBAL);
    ok = !JS_IsException(result);
    JS_FreeValue(cctx, result);
    return ok;
#endif
}

char* nd_qjs_get_last_error(void* ctx) {
#if !defined(NOVADESK_HAS_QUICKJS)
    (void)ctx;
    return nd_qjs_strdup("QuickJS support is not enabled in this build.");
#else
    JSContext* cctx;
    JSValue ex;
    const char* err;
    char* out;

    if (!ctx) return nd_qjs_strdup("QuickJS context is not initialized.");

    cctx = (JSContext*)ctx;
    ex = JS_GetException(cctx);
    err = JS_ToCString(cctx, ex);
    out = nd_qjs_strdup(err ? err : "Unknown QuickJS exception");

    if (err) {
        JS_FreeCString(cctx, err);
    }
    JS_FreeValue(cctx, ex);
    return out;
#endif
}

void nd_qjs_free_error(char* err) {
    if (err) {
        free(err);
    }
}
