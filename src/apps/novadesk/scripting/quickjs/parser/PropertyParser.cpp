#include "PropertyParser.h"

#include "rapidjson/document.h"

namespace novadesk::scripting::quickjs::parser {
std::string ParseMainFromMeta(const std::string& metaJson) {
    if (metaJson.empty()) {
        return {};
    }

    rapidjson::Document doc;
    if (doc.Parse(metaJson.c_str(), metaJson.size()).HasParseError() || !doc.IsObject()) {
        return {};
    }

    const auto mainIt = doc.FindMember("main");
    if (mainIt == doc.MemberEnd() || !mainIt->value.IsString()) {
        return {};
    }

    return mainIt->value.GetString();
}

void ParseWidgetWindowSize(JSContext* ctx, JSValueConst options, int& width, int& height) {
    width = 800;
    height = 600;

    if (!JS_IsObject(options)) {
        return;
    }

    JSValue widthVal = JS_GetPropertyStr(ctx, options, "width");
    JSValue heightVal = JS_GetPropertyStr(ctx, options, "height");

    if (!JS_IsUndefined(widthVal)) {
        JS_ToInt32(ctx, &width, widthVal);
    }
    if (!JS_IsUndefined(heightVal)) {
        JS_ToInt32(ctx, &height, heightVal);
    }

    JS_FreeValue(ctx, widthVal);
    JS_FreeValue(ctx, heightVal);
}
}  // namespace novadesk::scripting::quickjs::parser
