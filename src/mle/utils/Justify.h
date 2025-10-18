#pragma once

#include <iostream>

#include "Types.h"
#include "mle/core/Assert.h"
#include "mle/utils/Utils.h"

namespace mle {
template <typename T>
    requires std::is_arithmetic_v<T>
struct Justify {
    enum class LineMode : u8 { START, CENTER, END, SPACE_BETWEEN, SPACE_AROUND, SPACE_EVENLY };

    using Line = std::vector<T>;
    using Lines = std::vector<Line>;

    struct NoWrapRet {
        Line line{};
        T total_size{};
    };
    static NoWrapRet noWrap(std::span<T> sizes, T gap) {
        NoWrapRet ret;
        if (sizes.empty()) {
            return ret;
        }
        for (const T& s : sizes) {
            ret.line.push_back(ret.total_size);
            ret.total_size += s + gap;
        }
        ret.total_size -= gap;
        return ret;
    }

    static Lines wrapLines(std::span<T> sizes, T min_gap, LineMode mode, LineMode mode_last_line, T line_max_size) {
        Lines lines;
        if (sizes.empty() || 0 >= sizes.size()) {
            return lines;
        }

        usize start_index = 0;
        while (start_index < sizes.size()) {
            auto line = wrapOneLine(sizes.subspan(start_index), min_gap, mode, mode_last_line, line_max_size);
            lines.push_back(line);
            std::cout << lines.size() << " " << line.size() << "\n";
            start_index += line.size();
        }

        return lines;
    }

    static Line wrapOneLine(std::span<T> sizes, T min_gap, LineMode mode, LineMode mode_last_line, T line_max_size) {
        if (sizes.empty() || 0 >= sizes.size()) {
            return {};
        }
        if (sizes[0] > line_max_size) {
            return {0};
        }

        auto acc_size = T{0};
        auto acc_min_gap = T{0};
        usize count = 0;

        if (mode == LineMode::SPACE_AROUND || mode == LineMode::SPACE_EVENLY) {
            acc_min_gap += 2 * min_gap;
        }

        for (usize i = 0; i < sizes.size(); ++i) {
            const auto size = sizes[i];

            if (acc_size + acc_min_gap + size > line_max_size) {
                break;
            }

            acc_size += size;
            count++;

            if (acc_size + acc_min_gap + min_gap <= line_max_size) {
                acc_min_gap += min_gap;
            } else {
                break;
            }
        }

        bool is_last_line = (count == sizes.size());
        auto chosen_mode = is_last_line ? mode_last_line : mode;
        auto sub = sizes.subspan(0, count);

        return justifyLine(sub, chosen_mode, line_max_size, acc_size, min_gap * (sub.size() - 1), min_gap);
    }

    static Line justifyLine(std::span<T> sizes, LineMode mode, T line_size, T total_size, T acc_min_gap, T min_gap) {
        if (sizes.empty()) {
            return {};
        }

        switch (mode) {
            case LineMode::START: {
                Line line;
                T pos = 0;
                for (const T& s : sizes) {
                    line.push_back(pos);
                    pos += s + min_gap;
                }
                return line;
            }
            case LineMode::CENTER: {
                Line line;
                T gap = (line_size - (total_size + acc_min_gap)) / 2;
                T pos = gap;
                for (const T& s : sizes) {
                    line.push_back(pos);
                    pos += s + min_gap;
                }
                return line;
            }
            case LineMode::END: {
                Line line;
                T gap = line_size - (total_size + acc_min_gap);
                T pos = gap;
                for (const T& s : sizes) {
                    line.push_back(pos);
                    pos += s + min_gap;
                }
                return line;
            }
            case LineMode::SPACE_BETWEEN: {
                Line line;
                T gaps = sizes.size() > 1 ? sizes.size() - 1 : 1;
                T gap = (line_size - total_size) / gaps;
                T pos = 0;
                for (const T& s : sizes) {
                    line.push_back(pos);
                    pos += s + gap;
                }
                return line;
            }
            case LineMode::SPACE_AROUND: {
                Line line;
                T gaps = sizes.size();
                T gap = (line_size - total_size) / gaps;
                T pos = gap / 2;
                for (const T& s : sizes) {
                    line.push_back(pos);
                    pos += s + gap;
                }
                return line;
            }
            case LineMode::SPACE_EVENLY: {
                Line line;
                T gaps = sizes.size() + 1;
                T gap = (line_size - total_size) / gaps;
                T pos = gap;
                for (const T& s : sizes) {
                    line.push_back(pos);
                    pos += s + gap;
                }
                return line;
            }
        }

        MLE_UNREACHABLE;
    }
};
}  // namespace mle
