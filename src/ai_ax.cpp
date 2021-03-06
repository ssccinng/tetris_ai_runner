﻿
//Code by ax_pokl
//Modify by ZouZhiZhang

#include "tetris_core.h"
#include "ai_ax.h"
#include "integer_utils.h"

using namespace m_tetris;


namespace ai_ax
{

    bool AI::Status::operator < (Status const &other) const
    {
        return value < other.value;
    }

    void AI::init(m_tetris::TetrisContext const *context)
    {
        context_ = context;
        map_danger_data_.resize(context->type_max());
        for(size_t i = 0; i < context->type_max(); ++i)
        {
            TetrisMap map(context->width(), context->height());
            TetrisNode const *node = context->generate(i);
            node->attach(map);
            std::memcpy(map_danger_data_[i].data, &map.row[map.height - 4], sizeof map_danger_data_[i].data);
            for(int y = 0; y < 3; ++y)
            {
                map_danger_data_[i].data[y + 1] |= map_danger_data_[i].data[y];
            }
        }
        col_mask_ = context->full() & ~1;
        row_mask_ = context->full();
    }

    std::string AI::ai_name() const
    {
        return "Tetris_ax_C ZZZ Mod v1.2";
    }

    AI::Result AI::eval(TetrisNode const *node, TetrisMap const &map, TetrisMap const &src_map, size_t clear) const
    {
        //消行数
        double LandHeight = node->status.y + 1;
        //设置左中右平衡破缺参数
        double Middle = std::abs((node->status.x + 1) * 2 - map.width);
        //当前块行数
        double EraseCount = clear;

        const int width_m1 = map.width - 1;
        //行列变换
        int ColTrans = 2 * (map.height - map.roof);
        int RowTrans = ZZZ_BitCount(row_mask_ ^ map.row[0]) + ZZZ_BitCount(map.roof == map.height ? ~row_mask_ & map.row[map.roof - 1] : map.row[map.roof - 1]);
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
            ColTrans += ZZZ_BitCount((map.row[y] ^ (map.row[y] << 1)) & col_mask_);
            if(y != 0)
            {
                RowTrans += ZZZ_BitCount(map.row[y - 1] ^ map.row[y]);
            }
        }
        struct
        {
            //洞数
            int HoleCount;
            //洞行数
            int HoleLine;
            //最高洞行数
            int HolePosy;
            //最高洞上方块数
            int HolePiece;

            //洞深,井深
            int HoleDepth;
            int WellDepth;

            //洞计数,井计数
            int HoleNum[32];
            int WellNum[32];

            uint32_t LineCoverBits;
            uint32_t TopHoleBits;
        } v;
        std::memset(&v, 0, sizeof v);

        for(int y = map.roof - 1; y >= 0; --y)
        {
            v.LineCoverBits |= map.row[y];
            int LineHole = v.LineCoverBits ^ map.row[y];
            if(LineHole != 0)
            {
                v.HoleCount += ZZZ_BitCount(LineHole);
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
            //从最高有洞行上一行开始往上厉遍
            for(int y = v.HolePosy; y < map.roof; ++y)
            {
                uint32_t CheckLine = v.TopHoleBits & map.row[y];
                if(CheckLine == 0)
                {
                    break;
                }
                v.HolePiece += (y + 1) * ZZZ_BitCount(CheckLine);
            }
        }

        //死亡警戒
        int BoardDeadZone = map_in_danger_(map);

        Result result;
        result.land_point = (0
                             - LandHeight * 1750 / map.height
                             + Middle * 2
                             + EraseCount * 60
                             );
        result.map = (0
                      - ColTrans * 80
                      - RowTrans * 80
                      - v.HoleCount * 60
                      - v.HoleLine * 380
                      - v.WellDepth * 100
                      - v.HoleDepth * 40
                      - v.HolePiece * 5
                      - BoardDeadZone * 50000
                      );
        return result;
    }

    AI::Status AI::get(Result const &eval_result, size_t depth, Status const &status) const
    {
        Status result;
        result.land_point = eval_result.land_point + status.land_point;
        result.value = result.land_point / depth + eval_result.map;
        return result;
    }

    AI::Status AI::iterate(Status const **status, size_t status_length) const
    {
        Status result;
        result.land_point = 0;
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

    size_t AI::map_in_danger_(m_tetris::TetrisMap const &map) const
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
