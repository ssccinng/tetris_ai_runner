
//by ZouZhiZhang

#include "tetris_core.h"
#include "integer_utils.h"
#include "ai_zzz.h"
#include <cstdint>

using namespace m_tetris;
using namespace zzz;

namespace
{
    enum ItemType
    {
        a3, a2, a1, m3, m2, m1, sf, ss
    };
    struct Item
    {
        int16_t col, type;
    };
    const Item ItemTable[20][7] =
    {
#define I(a,b) {0x##a-1,b}
        //LLLLLLL   JJJJJJJ   TTTTTTT   OOOOOOO   IIIIIII   ZZZZZZZ   SSSSSSS//
        {I(A, a2), I(B, a3), I(C, sf), I(1, a1), I(2, a2), I(3, a3), I(4, m1)},
        {I(4, a1), I(3, a2), I(2, a3), I(1, sf), I(C, a1), I(B, a2), I(A, a3)},
        {I(8, ss), I(9, a1), I(A, a2), I(B, a3), I(C, sf), I(1, a1), I(2, a2)},
        {I(6, m3), I(5, m2), I(4, a1), I(3, a2), I(2, a3), I(1, sf), I(C, a1)},
        {I(6, m2), I(7, m3), I(8, ss), I(9, a1), I(A, a2), I(B, a3), I(C, sf)},
        {I(8, a1), I(7, m1), I(6, m1), I(5, m1), I(4, a1), I(3, a2), I(2, a3)},
        {I(4, sf), I(5, a1), I(6, m3), I(7, m2), I(8, m1), I(9, a1), I(A, a2)},
        {I(A, a3), I(9, sf), I(8, a1), I(7, m3), I(6, m1), I(5, ss), I(4, a1)},
        {I(2, a2), I(3, a3), I(4, sf), I(5, a1), I(6, m1), I(7, m3), I(8, m2)},
        {I(C, a1), I(B, a2), I(A, a3), I(9, sf), I(8, a1), I(7, m2), I(6, m3)},
        {I(C, sf), I(1, a1), I(2, a2), I(3, a3), I(4, sf), I(5, a1), I(6, m1)},
        {I(2, a3), I(1, sf), I(C, a1), I(B, a2), I(A, a3), I(9, sf), I(8, a1)},
        {I(A, ss), I(B, a3), I(C, sf), I(1, a1), I(2, a2), I(3, a3), I(4, sf)},
        {I(4, m3), I(3, m2), I(2, a3), I(1, sf), I(C, a1), I(B, a2), I(A, a3)},
        {I(8, m2), I(9, m3), I(A, ss), I(B, a3), I(C, sf), I(1, a1), I(2, a2)},
        {I(6, a3), I(5, m1), I(4, m1), I(3, m1), I(2, a3), I(1, sf), I(C, a1)},
        {I(6, a2), I(7, a3), I(8, m3), I(9, m2), I(A, m1), I(B, a3), I(C, sf)},
        {I(8, a1), I(7, a2), I(6, a3), I(5, m3), I(4, m1), I(3, ss), I(2, a3)},
        {I(4, sf), I(5, a1), I(6, a2), I(7, a3), I(8, m1), I(9, m3), I(A, m2)},
        {I(A, a3), I(9, sf), I(8, a1), I(7, a2), I(6, a3), I(5, m2), I(4, m3)},
        //LLLLLLL   JJJJJJJ   TTTTTTT   OOOOOOO   IIIIIII   ZZZZZZZ   SSSSSSS//
#undef I
    };

}

namespace ai_zzz
{
    namespace qq
    {

        bool Attack::Status::operator < (Status const &other) const
        {
            return value < other.value;
        }


        void Attack::init(m_tetris::TetrisContext const *context, Config const *config)
        {
            context_ = context;
            config_ = config;
            check_line_1_end_ = check_line_1_;
            check_line_2_end_ = check_line_2_;
            const int full = context->full();
            for(int x = 0; x < context->width(); ++x)
            {
                *check_line_1_end_++ = full & ~(1 << x);
            }
            for(int x = 0; x < context->width() - 1; ++x)
            {
                *check_line_2_end_++ = full & ~(3 << x);
            }
            std::sort(check_line_1_, check_line_1_end_);
            std::sort(check_line_2_, check_line_2_end_);
            map_danger_data_.resize(context->type_max());
            for(size_t i = 0; i < context->type_max(); ++i)
            {
                TetrisMap map(context->width(), context->height());
                TetrisNode const *node = context->generate(i);
                node->attach(map);
                memcpy(map_danger_data_[i].data, &map.row[map.height - 4], sizeof map_danger_data_[i].data);
                for(int y = 0; y < 3; ++y)
                {
                    map_danger_data_[i].data[y + 1] |= map_danger_data_[i].data[y];
                }
            }
            col_mask_ = context->full() & ~1;
            row_mask_ = context->full();
        }

        std::string Attack::ai_name() const
        {
            return "AX Attack v0.1";
        }

        Attack::Result Attack::eval(TetrisNode const *node, TetrisMap const &map, TetrisMap const &src_map, size_t clear) const
        {
            double LandHeight = node->row + node->height;
            double Middle = std::abs((node->status.x + 1) * 2 - map.width);
            double EraseCount = clear;
            double DeadZone = node->row + node->height == map.height ? 500000. : 0;
            double BoardDeadZone = map_in_danger_(map);
            if(map.roof == map.height)
            {
                BoardDeadZone += 70;
            }

            const int width_m1 = map.width - 1;
            int ColTrans = 2 * (map.height - map.roof);
            int RowTrans = map.roof == map.height ? 0 : map.width;
            for(int y = 0; y < map.roof; ++y)
            {
                if(!map.full(0, y))
                {
                    ++ColTrans;
                }
                if(!map.full(width_m1, y))
                {
                    ++ColTrans;
                }
                ColTrans += BitCount((map.row[y] ^ (map.row[y] << 1)) & col_mask_);
                if(y != 0)
                {
                    RowTrans += BitCount(map.row[y - 1] ^ map.row[y]);
                }
            }
            RowTrans += BitCount(row_mask_ & ~map.row[0]);
            RowTrans += BitCount(map.roof == map.height ? row_mask_ & ~map.row[map.roof - 1] : map.row[map.roof - 1]);
            struct
            {
                int HoleCount;
                int HoleLine;
                int HolePosy;
                int HolePiece;

                int HoleDepth;
                int WellDepth;

                int HoleNum[32];
                int WellNum[32];

                double AttackDepth;

                int Danger;
                int LineCoverBits;
                int TopHoleBits;
            } v;
            memset(&v, 0, sizeof v);

            for(int y = map.roof - 1; y >= 0; --y)
            {
                v.LineCoverBits |= map.row[y];
                int LineHole = v.LineCoverBits ^ map.row[y];
                if(LineHole != 0)
                {
                    v.HoleCount += BitCount(LineHole);
                    v.HoleLine++;
                    if(v.HolePosy == 0)
                    {
                        v.HolePosy = y + 1;
                        v.TopHoleBits = LineHole;
                    }
                }
                for(int x = 1; x < width_m1; ++x)
                {
                    if((LineHole >> x) & 1)
                    {
                        v.HoleDepth += ++v.HoleNum[x];
                    }
                    else
                    {
                        v.HoleNum[x] = 0;
                    }
                    if(((v.LineCoverBits >> (x - 1)) & 7) == 5)
                    {
                        v.WellDepth += ++v.WellNum[x];
                    }
                }
                if(LineHole & 1)
                {
                    v.HoleDepth += ++v.HoleNum[0];
                }
                else
                {
                    v.HoleNum[0] = 0;
                }
                if((v.LineCoverBits & 3) == 2)
                {
                    v.WellDepth += ++v.WellNum[0];
                }
                if((LineHole >> width_m1) & 1)
                {
                    v.HoleDepth += ++v.HoleNum[width_m1];
                }
                else
                {
                    v.HoleNum[width_m1] = 0;
                }
                if(((v.LineCoverBits >> (width_m1 - 1)) & 3) == 1)
                {
                    v.WellDepth += ++v.WellNum[width_m1];
                }
            }
            if(v.HolePosy != 0)
            {
                for(int y = v.HolePosy; y < map.roof; ++y)
                {
                    int CheckLine = v.TopHoleBits & map.row[y];
                    if(CheckLine == 0)
                    {
                        break;
                    }
                    v.HolePiece += (y + 1) * BitCount(CheckLine);
                }
            }
            int low_x;
            if((config_->mode & 1) == 0)
            {
                low_x = 1;
                for(int x = 2; x < width_m1; ++x)
                {
                    if(map.top[x] < map.top[low_x])
                    {
                        low_x = x;
                    }
                }
                if(map.top[0] < map.top[low_x])
                {
                    low_x = 0;
                }
                if(map.top[width_m1] < map.top[low_x])
                {
                    low_x = width_m1;
                }
            }
            else
            {
                low_x = (map.top[width_m1 - 3] <= map.top[width_m1 - 4]) ? width_m1 - 3 : width_m1 - 4;
                if(map.top[width_m1 - 2] <= map.top[low_x])
                {
                    low_x = width_m1 - 2;
                }
            }
            int low_y = map.top[low_x];
            for(int y = map.roof - 1; y >= low_y; --y)
            {
                if(std::binary_search<uint32_t const *>(check_line_1_, check_line_1_end_, map.row[y]))
                {
                    if(y + 1 < map.height && std::binary_search<uint32_t const *>(check_line_2_, check_line_2_end_, map.row[y + 1]))
                    {
                        v.AttackDepth += 20;
                    }
                    else
                    {
                        v.AttackDepth += 16;
                    }
                    for(--y; y >= low_y; --y)
                    {
                        if(std::binary_search<uint32_t const *>(check_line_1_, check_line_1_end_, map.row[y]))
                        {
                            v.AttackDepth += 3;
                        }
                        else
                        {
                            v.AttackDepth -= 5;
                        }
                    }
                    break;
                }
                else
                {
                    v.AttackDepth -= 2;
                }
            }
            v.Danger = 5 - std::min(map.height - low_y, 5);
            if(v.Danger > 0 && v.AttackDepth < 20)
            {
                v.AttackDepth = 0;
            }

            Result result;
            result.land_point = (0.
                                 - LandHeight * 16
                                 + Middle  * 0.2
                                 + EraseCount * 6
                                 - DeadZone
                                 - BoardDeadZone * 500000
                                 );
            result.map = (0.
                          - ColTrans * 32
                          - RowTrans * 32
                          - v.HoleCount * 400
                          - v.HoleLine * 38
                          - v.WellDepth * 16
                          - v.HoleDepth * 4
                          - v.HolePiece * 2
                          + v.AttackDepth * 100
                          );
            result.clear = clear;
            result.danger = v.Danger;
            return result;
        }

        Attack::Status Attack::get(Result const &eval_result, size_t depth, char hold, Status const &status) const
        {

            Status result = status;
            result.land_point += eval_result.land_point;
            double length_rate = 10. / depth;
            switch(eval_result.clear)
            {
            case 0:
                break;
            case 1:
            case 2:
                result.rubbish += eval_result.clear;
                break;
            case 3:
                if(config_->mode != 0)
                {
                    result.attack += 12;
                    break;
                }
            default:
                result.attack += (eval_result.clear * 10 * length_rate);
                break;
            }
            result.value = (0.
                            + result.land_point / depth
                            + eval_result.map
                            - result.rubbish * (eval_result.danger > 0 ? -100 : 640)
                            + result.attack * 100
                            );
            return result;
        }

        size_t Attack::map_in_danger_(m_tetris::TetrisMap const &map) const
        {
            size_t danger = 0;
            for(size_t i = 0; i < context_->type_max(); ++i)
            {
                if(map_danger_data_[i].data[0] & map.row[map.height - 4] || map_danger_data_[i].data[1] & map.row[map.height - 3] || map_danger_data_[i].data[2] & map.row[map.height - 2] || map_danger_data_[i].data[3] & map.row[map.height - 1])
                {
                    ++danger;
                }
            }
            return danger;
        }
    }

    bool Dig::Status::operator < (Status const &other) const
    {
        return value < other.value;
    }

    void Dig::init(m_tetris::TetrisContext const *context)
    {
        context_ = context;
        map_danger_data_.resize(context->type_max());
        for(size_t i = 0; i < context->type_max(); ++i)
        {
            TetrisMap map(context->width(), context->height());
            TetrisNode const *node = context->generate(i);
            node->attach(map);
            memcpy(map_danger_data_[i].data, &map.row[map.height - 4], sizeof map_danger_data_[i].data);
            for(int y = 0; y < 3; ++y)
            {
                map_danger_data_[i].data[y + 1] |= map_danger_data_[i].data[y];
            }
        }
        col_mask_ = context->full() & ~1;
        row_mask_ = context->full();
    }

    std::string Dig::ai_name() const
    {
        return "ZZZ Dig v0.2";
    }

    Dig::Result Dig::eval(m_tetris::TetrisNode const *node, m_tetris::TetrisMap const &map, m_tetris::TetrisMap const &src_map, size_t clear) const
    {
        double LandHeight = node->status.y + 1;
        double Middle = std::abs((node->status.x + 1) * 2 - map.width);
        double EraseCount = clear;

        const int width_m1 = map.width - 1;
        int ColTrans = 2 * (map.height - map.roof);
        int RowTrans = zzz::BitCount(row_mask_ ^ map.row[0]) + zzz::BitCount(map.roof == map.height ? ~row_mask_ & map.row[map.roof - 1] : map.row[map.roof - 1]);
        for(int y = 0; y < map.roof; ++y)
        {
            if(!map.full(0, y))
            {
                ++ColTrans;
            }
            if(!map.full(width_m1, y))
            {
                ++ColTrans;
            }
            ColTrans += zzz::BitCount((map.row[y] ^ (map.row[y] << 1)) & col_mask_);
            if(y != 0)
            {
                RowTrans += zzz::BitCount(map.row[y - 1] ^ map.row[y]);
            }
        }
        struct
        {
            int HoleCount;
            int HoleLine;

            int HoleDepth;
            int WellDepth;

            int HoleNum[32];
            int WellNum[32];

            int LineCoverBits;
            
            int HoleBits[8];
            int HolePosy[8];
            int ClearWidth[8];
            int PosyIndex;
        } v;
        memset(&v, 0, sizeof v);

        for(int y = map.roof - 1; y >= 0; --y)
        {
            v.LineCoverBits |= map.row[y];
            int LineHole = v.LineCoverBits ^ map.row[y];
            if(LineHole != 0)
            {
                v.HoleCount += zzz::BitCount(LineHole);
                v.HoleLine++;
                if(v.PosyIndex < 8 && v.HolePosy[v.PosyIndex] == 0)
                {
                    v.HolePosy[v.PosyIndex] = y + 1;
                    v.HoleBits[v.PosyIndex] = LineHole;
                    ++v.PosyIndex;
                }
            }
            for(int x = 1; x < width_m1; ++x)
            {
                if((LineHole >> x) & 1)
                {
                    v.HoleDepth += ++v.HoleNum[x];
                }
                else
                {
                    v.HoleNum[x] = 0;
                }
                if(((v.LineCoverBits >> (x - 1)) & 7) == 5)
                {
                    v.WellDepth += ++v.WellNum[x];
                }
            }
            if(LineHole & 1)
            {
                v.HoleDepth += ++v.HoleNum[0];
            }
            else
            {
                v.HoleNum[0] = 0;
            }
            if((v.LineCoverBits & 3) == 2)
            {
                v.WellDepth += ++v.WellNum[0];
            }
            if((LineHole >> width_m1) & 1)
            {
                v.HoleDepth += ++v.HoleNum[width_m1];
            }
            else
            {
                v.HoleNum[width_m1] = 0;
            }
            if(((v.LineCoverBits >> (width_m1 - 1)) & 3) == 1)
            {
                v.WellDepth += ++v.WellNum[width_m1];
            }
        }
        for(int i = 0; i < v.PosyIndex; ++i)
        {
            for(int y = v.HolePosy[i]; y < map.roof; ++y)
            {
                int CheckLine = v.HoleBits[i] & map.row[y];
                if(CheckLine == 0)
                {
                    break;
                }
                v.ClearWidth[i] += (y + 1) * zzz::BitCount(CheckLine);
            }
        }

        //��������
        int BoardDeadZone = map_in_danger_(map);

        Result result;
        result.land_point = (0
                             - LandHeight * 1750 / map.height
                             + Middle * 2
                             + EraseCount * 60
                             );
        result.map = (0.
                      - ColTrans * 80
                      - RowTrans * 80
                      - v.HoleCount * 60
                      - v.HoleLine * 380
                      - v.WellDepth * 100
                      - v.HoleDepth * 40
                      - v.ClearWidth[0] * 4
                      - v.ClearWidth[1] * 1
                      - v.ClearWidth[2] * 0.25
                      - v.ClearWidth[3] * 0.125
                      - v.ClearWidth[4] * 0.0625
                      - v.ClearWidth[5] * 0.03125
                      - v.ClearWidth[6] * 0.015625
                      - v.ClearWidth[7] * 0.0078125
                      - BoardDeadZone * 50000
                      );
        return result;
    }

    Dig::Status Dig::get(Result const &eval_result, size_t depth, char hold, Status const &status) const
    {
        Status result;
        result.land_point = eval_result.land_point + status.land_point;
        result.value = result.land_point / depth + eval_result.map;
        return result;
    }

    size_t Dig::map_in_danger_(m_tetris::TetrisMap const &map) const
    {
        size_t danger = 0;
        for(size_t i = 0; i < context_->type_max(); ++i)
        {
            if(map_danger_data_[i].data[0] & map.row[map.height - 4] || map_danger_data_[i].data[1] & map.row[map.height - 3] || map_danger_data_[i].data[2] & map.row[map.height - 2] || map_danger_data_[i].data[3] & map.row[map.height - 1])
            {
                ++danger;
            }
        }
        return danger;
    }

    bool TOJ::Status::operator < (Status const &other) const
    {
        return value < other.value;
    }

    void TOJ::init(m_tetris::TetrisContext const *context, Config const *config)
    {
        context_ = context;
        config_ = config;
        col_mask_ = context->full() & ~1;
        row_mask_ = context->full();
        danger_line_ = context->height();
        danger_data_ = 0;
        for(size_t i = 0; i < context->type_max(); ++i)
        {
            TetrisNode const *node = context->generate(i);
            danger_line_ = std::min<int>(danger_line_, node->row);
            for(int x = node->col; x < node->col + node->width; ++x)
            {
                danger_data_ |= 1 << x;
            }
        }
        full_count_ = context->width() * (danger_line_ - 1);
    }

    std::string TOJ::ai_name() const
    {
        return "ZZZ TOJ v0.5";
    }

    TOJ::Result TOJ::eval(TetrisNodeEx &node, m_tetris::TetrisMap const &map, m_tetris::TetrisMap const &src_map, size_t clear) const
    {
        Result result;
        result.eval = 0;
        result.count = 0;
        const int width = map.width;

        for(int x = 0; x < width; ++x)
        {
            result.count += map.top[x];
            for(int y = 0; y < map.roof; ++y)
            {
                if(map.full(x, y))
                {
                    result.eval -= 2 * (y + 1);
                    continue;
                }
                if(x == width - 1 || map.full(x + 1, y))
                {
                    result.eval -= 3 * (y + 1);
                }
                if(x == 0 || map.full(x - 1, y))
                {
                    result.eval -= 3 * (y + 1);
                }
                if(map.full(x, y + 1))
                {
                    result.eval -= 10 * (y + 1);
                    if(map.full(x, y + 2))
                    {
                        result.eval -= 4;
                        if(map.full(x, y + 3))
                        {
                            result.eval -= 3;
                            if(map.full(x, y + 4))
                            {
                                result.eval -= 2;
                            }
                        }
                    }
                }
            }
        }
        result.clear = clear;
        int line;
        for(line = map.roof - 1; line > 0; --line)
        {
            if(map.row[line] & danger_data_)
            {
                break;
            }
        }
        result.safe = danger_line_ - (line + 2);
        if(clear > 0 && node.is_check && node.is_last_rotate)
        {
            if(clear == 1 && node.is_mini_ready)
            {
                node.type = TSpinType::TSpinMini;
            }
            else if(node.is_ready)
            {
                node.type = TSpinType::TSpin;
            }
            else
            {
                node.type = TSpinType::None;
            }
        }
        result.t_spin = node.type;
        result.t2_value = 0;
        result.t3_value = 0;
        bool finding2 = true;
        bool finding3 = true;
        for(int y = 0; (finding2 || finding3) && y < map.roof - 2; ++y)
        {
            int row0 = map.row[y];
            int row1 = map.row[y + 1];
            int row2 = y + 2 < map.height ? map.row[y + 2] : 0;
            int row3 = y + 3 < map.height ? map.row[y + 3] : 0;
            int row4 = y + 4 < map.height ? map.row[y + 4] : 0;
            for(int x = 0; finding2 && x < map.width - 2; ++x)
            {
                if(((row0 >> x) & 7) == 5)
                {
                    if(((row1 >> x) & 7) == 0)
                    {
                        if(BitCount(row0) == map.width - 1)
                        {
                            result.t2_value += 1;
                            if(BitCount(row1) == map.width - 3)
                            {
                                result.t2_value += 2;
                                int row2_check = (row2 >> x) & 7;
                                if(row2_check == 1 || row2_check == 4)
                                {
                                    result.t2_value += 2;
                                }
                                finding2 = false;
                            }
                        }
                    }
                }
            }
            for(int x = 0; finding3 && x < map.width - 3; ++x)
            {
                if(((row0 >> x) & 15) == 11)
                {
                    if(((row1 >> x) & 15) == 9)
                    {
                        if(BitCount(row0) == map.width - 1)
                        {
                            result.t3_value += 1;
                            if(BitCount(row1) == map.width - 2)
                            {
                                result.t3_value += 2;
                                if(((row2 >> x) & 15) == 11)
                                {
                                    result.t3_value += 1;
                                    if(BitCount(row2) == map.width - 1)
                                    {
                                        result.t3_value += 1;
                                        if(((row3 >> x) & 15) == 8)
                                        {
                                            result.t3_value += 3;
                                            int row4_check = ((row4 >> x) & 15);
                                            if(row4_check == 4 || row4_check == 12)
                                            {
                                                result.t3_value += 5;
                                            }
                                        }
                                    }
                                    finding3 = false;
                                }
                            }
                        }
                    }
                }
                if(((row0 >> x) & 15) == 13)
                {
                    if(((row1 >> x) & 15) == 9)
                    {
                        if(BitCount(row0) == map.width - 1)
                        {
                            result.t3_value += 1;
                            if(BitCount(row1) == map.width - 2)
                            {
                                result.t3_value += 2;
                                if(((row2 >> x) & 15) == 13)
                                {
                                    result.t3_value += 1;
                                    if(BitCount(row2) == map.width - 1)
                                    {
                                        result.t3_value += 1;
                                        if(((row3 >> x) & 15) == 1)
                                        {
                                            result.t3_value += 3;
                                            int row4_check = ((row4 >> x) & 15);
                                            if(row4_check == 3 || row4_check == 1)
                                            {
                                                result.t3_value += 5;
                                            }
                                        }
                                    }
                                    finding3 = false;
                                }
                            }
                        }
                    }
                }
            }
        }
        return result;
    }

    TOJ::Status TOJ::get(Result const &eval_result, size_t depth, char hold, Status const &status) const
    {
        Status result = status;
        result.value = eval_result.eval;
        int up = 0;
        if(eval_result.safe <= 0)
        {
            result.value -= 99999;
        }
        switch(eval_result.clear)
        {
        case 0:
            if(status.combo > 0 && status.combo < 3)
            {
                result.like -= 2;
            }
            result.combo = 0;
            if(status.under_attack > 0)
            {
                up = std::max(0, int(status.under_attack) - status.attack);
                if(up >= eval_result.safe)
                {
                    result.value -= 99999;
                }
                result.under_attack = 0;
            }
            break;
        case 1:
            if(eval_result.t_spin == TSpinType::TSpinMini)
            {
                result.attack += status.b2b ? 2 : 1;
            }
            else if(eval_result.t_spin == TSpinType::TSpin)
            {
                result.attack += status.b2b ? 3 : 2;
            }
            result.attack += config_->table[std::min(config_->table_max - 1, ++result.combo)];
            result.b2b = eval_result.t_spin != TSpinType::None;
            break;
        case 2:
            if(eval_result.t_spin != TSpinType::None)
            {
                result.like += 8;
                result.attack += status.b2b ? 5 : 4;
            }
            result.attack += config_->table[std::min(config_->table_max - 1, ++result.combo)];
            result.b2b = eval_result.t_spin != TSpinType::None;
            break;
        case 3:
            if(eval_result.t_spin != TSpinType::None)
            {
                result.like += 12;
                result.attack += status.b2b ? 8 : 6;
            }
            result.attack += config_->table[std::min(config_->table_max - 1, ++result.combo)] + 2;
            result.b2b = eval_result.t_spin != TSpinType::None;
            break;
        case 4:
            result.like += 8;
            result.attack += config_->table[std::min(config_->table_max - 1, ++result.combo)] + (status.b2b ? 5 : 4);
            result.b2b = true;
            break;
        }
        if(result.combo < 5)
        {
            result.like -= result.combo;
        }
        if(eval_result.count == 0 && up == 0)
        {
            result.like += 20;
            result.attack += 6;
        }
        switch(hold)
        {
        case 'T':
            if(eval_result.t_spin == TSpinType::None)
            {
                result.like += 4;
            }
            break;
        case 'I':
            if(eval_result.clear != 4)
            {
                result.like += 2;
            }
            break;
        }
        double rate = (1. / depth) + 1.;
        result.value += (0.
                         + result.attack * 160
                         + eval_result.t2_value * 128
                         + (eval_result.safe >= 12 && result.under_attack < 2 ? eval_result.t3_value * (result.b2b ? 128 : 80) : 0)
                         + (result.b2b ? 240 : 0)
                         + result.like * 24
                         ) * rate * std::max(0, full_count_ - eval_result.count - result.under_attack * context_->width()) / full_count_;
        return result;
    }

    bool C2::Status::operator < (Status const &other) const
    {
        return value < other.value;
    }


    void C2::init(m_tetris::TetrisContext const *context, Config const *config)
    {
        context_ = context;
        config_ = config;
        map_danger_data_.resize(context->type_max());
        for(size_t i = 0; i < context->type_max(); ++i)
        {
            TetrisMap map(context->width(), context->height());
            TetrisNode const *node = context->generate(i);
            node->move_down->attach(map);
            memcpy(map_danger_data_[i].data, &map.row[map.height - 4], sizeof map_danger_data_[i].data);
            for(int y = 0; y < 3; ++y)
            {
                map_danger_data_[i].data[y + 1] |= map_danger_data_[i].data[y];
            }
        }
        col_mask_ = context->full() & ~1;
        row_mask_ = context->full();
    }

    std::string C2::ai_name() const
    {
        return "C2 v0.1";
    }

    C2::Result C2::eval(TetrisNode const *node, TetrisMap const &map, TetrisMap const &src_map, size_t clear) const
    {
        double LandHeight = node->row + node->height;
        double Middle = std::abs((node->status.x + 1) * 2 - map.width);
        double EraseCount = clear;
        double BoardDeadZone = map_in_danger_(map);
        if(map.roof == map.height)
        {
            BoardDeadZone += 70;
        }

        const int width_m1 = map.width - 1;
        int ColTrans = 2 * (map.height - map.roof);
        int RowTrans = map.roof == map.height ? 0 : map.width;
        for(int y = 0; y < map.roof; ++y)
        {
            if(!map.full(0, y))
            {
                ++ColTrans;
            }
            if(!map.full(width_m1, y))
            {
                ++ColTrans;
            }
            ColTrans += BitCount((map.row[y] ^ (map.row[y] << 1)) & col_mask_);
            if(y != 0)
            {
                RowTrans += BitCount(map.row[y - 1] ^ map.row[y]);
            }
        }
        RowTrans += BitCount(row_mask_ & ~map.row[0]);
        RowTrans += BitCount(map.roof == map.height ? row_mask_ & ~map.row[map.roof - 1] : map.row[map.roof - 1]);
        struct
        {
            int HoleCount;
            int HoleLine;

            int WideWellDepth[6];
            int WellDepth[32];
            int WellDepthTotle;

            int LineCoverBits;
            int HoleBits0;
            int ClearWidth0;
            int HoleBits1;
            int ClearWidth1;
            int HoleBits2;
            int ClearWidth2;
        } v;
        memset(&v, 0, sizeof v);
        int HolePosy0 = -1;
        int HolePosy1 = -1;
        int HolePosy2 = -1;

        for(int y = map.roof - 1; y >= 0; --y)
        {
            v.LineCoverBits |= map.row[y];
            int LineHole = v.LineCoverBits ^ map.row[y];
            if(LineHole != 0)
            {
                v.HoleCount += BitCount(LineHole);
                v.HoleLine++;
                if(HolePosy0 == -1)
                {
                    HolePosy0 = y + 1;
                    v.HoleBits0 = LineHole;
                }
                else if(HolePosy1 == -1)
                {
                    HolePosy1 = y + 1;
                    v.HoleBits1 = LineHole;
                }
                else if(HolePosy2 == -1)
                {
                    HolePosy2 = y + 1;
                    v.HoleBits2 = LineHole;
                }
            }
            int WellWidth = 0;
            int MaxWellWidth = 0;
            for(int x = 0; x < map.width; ++x)
            {
                if((v.LineCoverBits >> x) & 1)
                {
                    if(WellWidth > MaxWellWidth)
                    {
                        MaxWellWidth = WellWidth;
                    }
                    WellWidth = 0;
                }
                else
                {
                    ++WellWidth;
                    if(x > 0 && x < width_m1)
                    {
                        if(((v.LineCoverBits >> (x - 1)) & 7) == 5)
                        {
                            v.WellDepthTotle += ++v.WellDepth[x];
                        }
                    }
                    else if(x == 0)
                    {
                        if((v.LineCoverBits & 3) == 2)
                        {
                            v.WellDepthTotle += ++v.WellDepth[0];
                        }
                    }
                    else
                    {
                        if(((v.LineCoverBits >> (width_m1 - 1)) & 3) == 1)
                        {
                            v.WellDepthTotle += ++v.WellDepth[width_m1];
                        }
                    }
                }
            }
            if(WellWidth > MaxWellWidth)
            {
                MaxWellWidth = WellWidth;
            }
            if(MaxWellWidth >= 1 && MaxWellWidth <= 6)
            {
                ++v.WideWellDepth[MaxWellWidth - 1];
            }
        }
        if(HolePosy0 >= 0)
        {
            for(int y = HolePosy0; y < map.roof; ++y)
            {
                int CheckLine = v.HoleBits0 & map.row[y];
                if(CheckLine == 0)
                {
                    break;
                }
                v.ClearWidth0 += (y + 1) * BitCount(CheckLine);
            }
            if(HolePosy1 >= 0)
            {
                for(int y = HolePosy1; y < map.roof; ++y)
                {
                    int CheckLine = v.HoleBits1 & map.row[y];
                    if(CheckLine == 0)
                    {
                        break;
                    }
                    v.ClearWidth1 += (y + 1) * BitCount(CheckLine);
                }
                if(HolePosy2 >= 0)
                {
                    for(int y = HolePosy2; y < map.roof; ++y)
                    {
                        int CheckLine = v.HoleBits2 & map.row[y];
                        if(CheckLine == 0)
                        {
                            break;
                        }
                        v.ClearWidth2 += (y + 1) * BitCount(CheckLine);
                    }
                }
            }
        }
        int low_x = 1;
        for(int x = 2; x < width_m1; ++x)
        {
            if(map.top[x] < map.top[low_x])
            {
                low_x = x;
            }
        }
        if(map.top[0] <= map.top[low_x])
        {
            low_x = 0;
        }
        if(map.top[width_m1] <= map.top[low_x])
        {
            low_x = width_m1;
        }
        int low_y = map.top[low_x];

        Result result;
        result.land_point = (0.
                             - LandHeight * 1750 / map.height
                             + Middle * 2
                             + EraseCount * 60
                             - BoardDeadZone * 50000000
                             );
        result.map = (0.
                      - ColTrans * 80
                      - RowTrans * 80
                      - v.HoleCount * 80
                      - v.HoleLine * 380
                      - v.ClearWidth0 * 8
                      - v.ClearWidth1 * 4
                      - v.ClearWidth2 * 1
                      - v.WellDepthTotle * 100
                      );
        if(config_->mode == 1)
        {
            int attack_well = std::min(4, v.WideWellDepth[0]);
            result.attack = (0.
                             + v.WideWellDepth[5] * 16
                             + v.WideWellDepth[4] * 24
                             + v.WideWellDepth[3] * 32
                             + v.WideWellDepth[2] * 40
                             + v.WideWellDepth[1] * 8
                             + attack_well * attack_well * 128
                             );
        }
        else
        {
            result.attack = 0;
        }
        result.clear = clear;
        result.low_y = low_y;
        result.count = map.count + v.HoleCount;
        result.soft_drop = !node->open(map);
        return result;
    }

    C2::Status C2::get(Result const &eval_result, size_t depth, char hold, Status const &status) const
    {
        Status result = status;
        result.land_point += eval_result.land_point;
        if(config_->mode == 1)
        {
            if(status.combo == 0)
            {
                result.land_point += eval_result.attack;
            }
            if(eval_result.clear > 0)
            {
                if(status.combo == 0)
                {
                    if(eval_result.clear == 4 && eval_result.count >= 84 - config_->safe * context_->width())
                    {
                        result.land_point += 8000;
                    }
                    else if(eval_result.clear == 3 && eval_result.count >= 92 - config_->safe * context_->width())
                    {
                        result.land_point += 4000;
                    }
                    else if(eval_result.low_y <= 5)
                    {
                        if(eval_result.count < 92 - config_->safe * context_->width())
                        {
                            result.land_point -= 4000;
                        }
                        else if(eval_result.clear < 3)
                        {
                            if(eval_result.count <= 100 - config_->safe * context_->width())
                            {
                                result.land_point -= 4000;
                            }
                            else if(eval_result.count <= 120 - config_->safe * context_->width())
                            {
                                result.land_point -= 2000;
                            }
                        }
                    }
                }
                else if(status.combo == 1)
                {
                    if(eval_result.clear == 4)
                    {
                        result.land_point += 2500;
                    }
                    else if(eval_result.clear == 3)
                    {
                        result.land_point += 1000;
                    }
                }
                else
                {
                    if(status.combo > 3 && eval_result.clear > 1 && eval_result.count <= 72 - config_->safe * context_->width())
                    {
                        result.land_point -= 1000;
                    }
                    if(status.combo < 6)
                    {
                        result.land_point += 2000;
                    }
                    else
                    {
                        result.land_point += status.combo * 10000;
                    }
                }
            }
            else if(status.combo > 0 && eval_result.count <= 64 - config_->safe * context_->width())
            {
                result.land_point -= 2000;
            }
            if(eval_result.clear > 0)
            {
                ++result.combo;
            }
        }
        if(eval_result.soft_drop)
        {
            result.land_point -= 800;
        }
        result.value = result.land_point / depth + eval_result.map;
        return result;
    }

    C2::Status C2::iterate(Status const **status, size_t status_length) const
    {
        Status result;
        result.land_point = 0;
        result.combo = 0;
        result.value = 0;
        for(size_t i = 0; i < status_length; ++i)
        {
            if(status[i] == nullptr)
            {
                result.value += -9999999999;
            }
            else
            {
                result.value += status[i]->value;
            }
        }
        result.value /= status_length;
        return result;
    }

    size_t C2::map_in_danger_(m_tetris::TetrisMap const &map) const
    {
        size_t danger = 0;
        for(size_t i = 0; i < context_->type_max(); ++i)
        {
            if(map_danger_data_[i].data[0] & map.row[map.height - 4] || map_danger_data_[i].data[1] & map.row[map.height - 3] || map_danger_data_[i].data[2] & map.row[map.height - 2] || map_danger_data_[i].data[3] & map.row[map.height - 1])
            {
                ++danger;
            }
        }
        if(map.row[17] != 0)
        {
            ++danger;
        }
        return danger;
    }
}