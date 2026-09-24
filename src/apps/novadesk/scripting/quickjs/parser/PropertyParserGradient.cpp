/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "PropertyParser.h"
#include "PropertyParserJs.h"

#include <algorithm>
#include <cmath>
#include <cwctype>
#include <vector>
#include <string>

#include "../../../shared/ColorUtil.h"

namespace PropertyParser {
using namespace Js;

namespace {

struct ParsedStop {
  COLORREF color = 0;
  BYTE alpha = 255;
  float position = 0.0f;
  bool hasPosition = false;
};

std::wstring Trim(const std::wstring &s) {
  size_t first = s.find_first_not_of(L" \t\r\n");
  if (first == std::wstring::npos)
    return L"";
  size_t last = s.find_last_not_of(L" \t\r\n");
  return s.substr(first, (last - first + 1));
}

bool ParseSingleStop(const std::wstring &raw, ParsedStop &out) {
  std::wstring s = Trim(raw);
  if (s.empty())
    return false;

  std::wstring colorStr;
  std::wstring posStr;

  size_t closeParen = s.find_last_of(L')');
  if (closeParen != std::wstring::npos) {
    colorStr = Trim(s.substr(0, closeParen + 1));
    posStr = Trim(s.substr(closeParen + 1));
  } else {
    size_t lastSpace = s.find_last_of(L" \t");
    if (lastSpace != std::wstring::npos) {
      std::wstring candidateColor = Trim(s.substr(0, lastSpace));
      std::wstring candidatePos = Trim(s.substr(lastSpace + 1));

      if (!candidatePos.empty() &&
          (iswdigit(candidatePos[0]) || candidatePos[0] == L'.' ||
           candidatePos[0] == L'-' || candidatePos[0] == L'+')) {
        colorStr = candidateColor;
        posStr = candidatePos;
      } else {
        colorStr = s;
        posStr = L"";
      }
    } else {
      colorStr = s;
      posStr = L"";
    }
  }

  if (!ColorUtil::ParseRGBA(colorStr, out.color, out.alpha)) {
    return false;
  }

  if (!posStr.empty()) {
    try {
      float val = 0.0f;
      size_t processed = 0;
      val = std::stof(posStr, &processed);
      if (processed > 0) {
        size_t pctPos = posStr.find(L'%', processed);
        if (pctPos != std::wstring::npos ||
            (processed < posStr.size() && posStr[processed] == L'%')) {
          out.position = val / 100.0f;
        } else {
          out.position = val;
        }
        out.hasPosition = true;
      }
    } catch (...) {
      out.hasPosition = false;
    }
  } else {
    out.hasPosition = false;
  }

  return true;
}

} // namespace

bool ParseGradientString(const std::wstring &str, GradientInfo &out) {
  if (str.empty())
    return false;
  std::wstring s = Trim(str);

  std::wstring lowerS = s;
  std::transform(lowerS.begin(), lowerS.end(), lowerS.begin(), ::towlower);

  size_t start = std::wstring::npos;
  if (lowerS.find(L"linear-gradient(") == 0) {
    out.type = GRADIENT_LINEAR;
    start = std::wstring(L"linear-gradient(").length();
  } else if (lowerS.find(L"lineargradient(") == 0) {
    out.type = GRADIENT_LINEAR;
    start = std::wstring(L"lineargradient(").length();
  } else if (lowerS.find(L"radial-gradient(") == 0) {
    out.type = GRADIENT_RADIAL;
    start = std::wstring(L"radial-gradient(").length();
  } else if (lowerS.find(L"radialgradient(") == 0) {
    out.type = GRADIENT_RADIAL;
    start = std::wstring(L"radialgradient(").length();
  } else {
    return false;
  }

  size_t end = lowerS.find_last_of(L')');
  if (end == std::wstring::npos || end <= start)
    return false;

  std::wstring content = s.substr(start, end - start);
  std::vector<std::wstring> parts;
  if (!TrySplitByComma(content, parts) || parts.empty())
    return false;

  int colorStartIndex = 0;
  if (out.type == GRADIENT_LINEAR) {
    std::wstring dir = Trim(parts[0]);
    std::wstring lowerDir = dir;
    std::transform(lowerDir.begin(), lowerDir.end(), lowerDir.begin(),
                   ::towlower);

    if (lowerDir == L"to top") {
      out.angle = 270.0f;
      colorStartIndex = 1;
    } else if (lowerDir == L"to bottom") {
      out.angle = 90.0f;
      colorStartIndex = 1;
    } else if (lowerDir == L"to right") {
      out.angle = 0.0f;
      colorStartIndex = 1;
    } else if (lowerDir == L"to left") {
      out.angle = 180.0f;
      colorStartIndex = 1;
    } else if (lowerDir == L"to top right" || lowerDir == L"to right top") {
      out.angle = 315.0f;
      colorStartIndex = 1;
    } else if (lowerDir == L"to bottom right" ||
               lowerDir == L"to right bottom") {
      out.angle = 45.0f;
      colorStartIndex = 1;
    } else if (lowerDir == L"to top left" || lowerDir == L"to left top") {
      out.angle = 225.0f;
      colorStartIndex = 1;
    } else if (lowerDir == L"to bottom left" ||
               lowerDir == L"to left bottom") {
      out.angle = 135.0f;
      colorStartIndex = 1;
    } else if (!lowerDir.empty()) {
      std::wstring numPart = lowerDir;
      float multiplier = 1.0f;
      if (numPart.length() >= 3 &&
          numPart.substr(numPart.length() - 3) == L"deg") {
        numPart = numPart.substr(0, numPart.length() - 3);
        multiplier = 1.0f;
      } else if (numPart.length() >= 3 &&
                 numPart.substr(numPart.length() - 3) == L"rad") {
        numPart = numPart.substr(0, numPart.length() - 3);
        multiplier = 180.0f / 3.14159265f;
      } else if (numPart.length() >= 4 &&
                 numPart.substr(numPart.length() - 4) == L"turn") {
        numPart = numPart.substr(0, numPart.length() - 4);
        multiplier = 360.0f;
      }

      numPart.erase(std::remove_if(numPart.begin(), numPart.end(), isspace),
                    numPart.end());
      if (!numPart.empty() &&
          (iswdigit(numPart[0]) || numPart[0] == L'-' || numPart[0] == L'.')) {
        try {
          size_t pos = 0;
          float val = std::stof(numPart, &pos);
          if (pos == numPart.length()) {
            out.angle = val * multiplier;
            colorStartIndex = 1;
          }
        } catch (...) {
        }
      }
    }
  } else {
    std::wstring shape = parts[0];
    std::transform(shape.begin(), shape.end(), shape.begin(), ::towlower);
    shape.erase(std::remove_if(shape.begin(), shape.end(), isspace),
                shape.end());

    if (shape == L"circle" || shape == L"ellipse") {
      out.shape = shape;
      colorStartIndex = 1;
    }
  }

  std::vector<ParsedStop> parsedStops;
  for (size_t i = colorStartIndex; i < parts.size(); i++) {
    ParsedStop stop;
    if (ParseSingleStop(parts[i], stop)) {
      parsedStops.push_back(stop);
    }
  }

  if (parsedStops.size() < 2)
    return false;

  // 1. If first stop has no position, default to 0.0
  if (!parsedStops.front().hasPosition) {
    parsedStops.front().position = 0.0f;
    parsedStops.front().hasPosition = true;
  }

  // 2. If last stop has no position, default to 1.0
  if (!parsedStops.back().hasPosition) {
    parsedStops.back().position = 1.0f;
    parsedStops.back().hasPosition = true;
  }

  // 3. Linearly distribute any unpositioned stops between positioned ones
  size_t lastPositioned = 0;
  for (size_t i = 1; i < parsedStops.size(); i++) {
    if (parsedStops[i].hasPosition) {
      if (parsedStops[i].position < parsedStops[lastPositioned].position) {
        parsedStops[i].position = parsedStops[lastPositioned].position;
      }
      if (i > lastPositioned + 1) {
        float startPos = parsedStops[lastPositioned].position;
        float endPos = parsedStops[i].position;
        size_t count = i - lastPositioned;
        for (size_t k = lastPositioned + 1; k < i; k++) {
          float frac = static_cast<float>(k - lastPositioned) /
                       static_cast<float>(count);
          parsedStops[k].position = startPos + frac * (endPos - startPos);
          parsedStops[k].hasPosition = true;
        }
      }
      lastPositioned = i;
    }
  }

  out.stops.clear();
  for (const auto &ps : parsedStops) {
    GradientStop s;
    s.color = ps.color;
    s.alpha = ps.alpha;
    s.position = (std::clamp)(ps.position, 0.0f, 1.0f);
    out.stops.push_back(s);
  }

  return true;
}

D2D1_CAP_STYLE GetCapStyle(const std::wstring &str) {
  if (str == L"Round")
    return D2D1_CAP_STYLE_ROUND;
  if (str == L"Square")
    return D2D1_CAP_STYLE_SQUARE;
  if (str == L"Triangle")
    return D2D1_CAP_STYLE_TRIANGLE;
  return D2D1_CAP_STYLE_FLAT;
}

D2D1_LINE_JOIN GetLineJoin(const std::wstring &str) {
  if (str == L"Bevel")
    return D2D1_LINE_JOIN_BEVEL;
  if (str == L"Round")
    return D2D1_LINE_JOIN_ROUND;
  if (str == L"MiterOrBevel")
    return D2D1_LINE_JOIN_MITER_OR_BEVEL;
  return D2D1_LINE_JOIN_MITER;
}
} // namespace PropertyParser
