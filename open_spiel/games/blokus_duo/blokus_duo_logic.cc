//
// Created by leoney on 05.11.25.
//
#include <cstdint>
#include <bitset>
#include <array>


#include "open_spiel/games/blokus_duo/blokus_duo_logic.h"
#include <unordered_map>
#include "spiel_utils.h"


namespace open_spiel {
    namespace blokus_duo {

        std::vector<int> edges_to_indices(const std::array<uint64_t, kNumBitboardParts>& edges)
        {
            std::vector<int> result;
            for (int part_index = 0; part_index < kNumBitboardParts; part_index++)
            {
                uint64_t part = edges[part_index];
                while (part)
                {
                    const int bit = __builtin_ctzll(part);
                    result.push_back(part_index * 64 + bit);
                    part &= part - 1;
                }
            }
            return result;
        }

        bool HasPolyomino(PolyominoType type, uint32_t polyomino_mask)
        {
            const int index = static_cast<int>(type);
            return polyomino_mask >> index & 1u;
        }

        ShiftedMask shifted_polyomino(const uint64_t polyomino, const int pos)
        {
            // if (pos <0)
            //     throw std::runtime_error("pos must be greater than 0");
            constexpr int kBitsPerPart = 64;
            ShiftedMask out{};
            out.part_index = pos / kBitsPerPart;
            if (polyomino == 255)
            {
                for (int i = 0; i < 5; ++i)
                {
                    const int bit_index = pos + i * 16; // jede Reihe ist 16 Bits entfernt
                    if (bit_index >= 256) break; // außerhalb des Spielfelds -> abbrechen

                    const int part = bit_index / kBitsPerPart;
                    const int bit_in_part = bit_index % kBitsPerPart;

                    if (part == out.part_index)
                        out.first_part |= 1ULL << bit_in_part;
                    else if (part == out.part_index + 1 && part < kNumBitboardParts)
                        out.second_part |= 1ULL << bit_in_part;
                }
            }
            else
            {
                const int shift_low = pos % kBitsPerPart;
                const int shift_high = kBitsPerPart - shift_low;
                out.first_part = polyomino << shift_low;
                if (shift_low != 0 && out.part_index + 1 < kNumBitboardParts)
                    out.second_part = polyomino >> shift_high;
            }
            return out;
        }

        bool is_colliding_with_border(
            const ShiftedMask& shifted_mask,
            const std::array<uint64_t, kNumBitboardParts>& border
        )
        {
            const bool border_collision =
                (border[shifted_mask.part_index] & shifted_mask.first_part) != 0 ||
                (shifted_mask.part_index + 1 < kNumBitboardParts &&
                    (border[shifted_mask.part_index + 1] & shifted_mask.second_part) != 0);
            return border_collision;
        }

        bool is_colliding(
            const ShiftedMask& shifted_mask,
            const std::array<uint64_t, kNumBitboardParts>& board
        )
        {
            const bool collision =
                (board[shifted_mask.part_index] & shifted_mask.first_part) != 0 ||
                (shifted_mask.part_index + 1 < kNumBitboardParts &&
                    (board[shifted_mask.part_index + 1] & shifted_mask.second_part) != 0);

            return collision;
        }

        bool is_in_edges(
            const ShiftedMask& shifted_mask,
            const std::array<uint64_t, kNumBitboardParts>& edges
        )
        {
            const bool in_edges =
                (edges[shifted_mask.part_index] & shifted_mask.first_part) != 0 ||
                (shifted_mask.part_index + 1 < kNumBitboardParts &&
                    (edges[shifted_mask.part_index + 1] & shifted_mask.second_part) != 0);

            return in_edges;
        }

        bool is_touching_own_polyominos(
            const ShiftedMask& shifted_mask,
            const std::array<uint64_t, kNumBitboardParts>& own_board)
        {
            std::array<uint64_t, kNumBitboardParts> edge_neighbors{};

            for (int i = 0; i < kNumBitboardParts; ++i)
            {
                const uint64_t part = own_board[i];
                const uint64_t left_shifted = part << 1 & 0xFFFEFFFEFFFEFFFEULL;
                const uint64_t right_shifted = part >> 1 & 0x7FFF7FFF7FFF7FFFULL;
                const uint64_t lower = i > 0 ? own_board[i - 1] >> 48 : 0ULL;
                const uint64_t down = part << 16 | lower;
                const uint64_t upper = i < 3 ? own_board[i + 1] << 48 : 0ULL;
                const uint64_t up  = part >> 16 | upper;

                edge_neighbors[i] |= left_shifted | right_shifted | up | down;
            }

            const bool touching =
                (edge_neighbors[shifted_mask.part_index] & shifted_mask.first_part) != 0 ||
                (shifted_mask.part_index + 1 < kNumBitboardParts &&
                    (edge_neighbors[shifted_mask.part_index + 1] & shifted_mask.second_part) != 0);

            return touching;
        }


        bool is_placement_legal(
            const std::array<uint64_t, kNumBitboardParts>& board,
            const std::array<uint64_t, kNumBitboardParts>& border,
            const std::array<uint64_t, kNumBitboardParts>& edges,
            const std::array<uint64_t, kNumBitboardParts>& own_board,
            const ShiftedMask& shifted_mask
        )
        {
            if (is_colliding(shifted_mask, board))return false;
            if (is_colliding_with_border(shifted_mask, border))return false;
            if (is_touching_own_polyominos(shifted_mask, own_board)) return false;
            if (!is_in_edges(shifted_mask, edges))return false;
            return true;
        }

std::vector<Action> LegalActions(
    const std::array<uint64_t, kNumBitboardParts>& board,
    const std::array<uint64_t, kNumBitboardParts>& player_board,
    const std::array<uint64_t, kNumBitboardParts>& border,
    const std::array<uint64_t, kNumBitboardParts>& edges,
    const std::unordered_map<PolyominoType, PolyominoBitboards>& all_polyomino_variants,
    const std::unordered_map<PolyominoType, std::vector<std::vector<int>>>& all_polyomino_indexes,
    const uint32_t polyomino_mask
    )
{
    const std::vector<int> indices_of_edges = edges_to_indices(edges);
    std::vector<Action> actions;

    auto has_piece = [&](PolyominoType type) {
        const int index = static_cast<int>(type);
        return polyomino_mask >> index & 1u;
    };
    for (const auto& [type, variants] : all_polyomino_variants)
    {
        if (has_piece(type))
        {
            for (int v = 0; v < variants.size(); ++v)
            {
                const uint64_t variant_mask = variants[v];
                std::vector<int> variant_indices = all_polyomino_indexes.at(type).at(v);
                std::bitset<256> tested_positions;

                for (const int edge_index: indices_of_edges)
                {
                    for (const int variant_index : variant_indices)
                    {
                        const int pos = edge_index - variant_index;
                        if ( pos < 0 || tested_positions.test(pos) ) continue;
                        tested_positions.set(pos);
                        ShiftedMask shifted = shifted_polyomino(variant_mask, pos);
                        if (is_placement_legal(board, border, edges, player_board, shifted))
                        {
                            actions.push_back(Action{
                                .type = type,
                                .variant_index = v,
                                .position = pos,
                                .shifted = shifted // optional
                            });
                        }
                    }
                }
            }
        }
    }
    return actions;
}
std::vector<Action> GetAllDistinctActions(
    const std::unordered_map<PolyominoType, PolyominoBitboards>& all_polyomino_variants,
    const std::unordered_map<PolyominoType, std::vector<std::vector<int>>>& all_polyomino_indexes
    )
{
    const std::array<uint64_t, kNumBitboardParts>& board = {0ULL, 0ULL, 0ULL, 0ULL};
    const std::array<uint64_t, kNumBitboardParts>& player_board = {0ULL, 0ULL, 0ULL, 0ULL};
    const std::array<uint64_t, kNumBitboardParts>& border = {
        9223794255762423807ULL, 9223794255762391041ULL,
        9223794255762391041ULL, 18446603342663745537ULL
    };
    const std::array<uint64_t, kNumBitboardParts>& edges = {~0ULL,~0ULL,~0ULL,~0ULL};
    const std::vector<int> indices_of_edges = edges_to_indices(edges);
    std::vector<Action> all_actions;

    for (const auto& [type, variants] : all_polyomino_variants)
    {
        for (int v = 0; v < variants.size(); ++v)
        {
            const uint64_t variant_mask = variants[v];
            std::vector<int> variant_indices = all_polyomino_indexes.at(type).at(v);
            std::bitset<256> tested_positions;

            for (const int edge_index : indices_of_edges)
            {
                for (const int variant_index : variant_indices)
                {
                    const int pos = edge_index - variant_index;
                    if (pos < 0 || tested_positions.test(pos)) continue;
                    tested_positions.set(pos);
                    ShiftedMask shifted = shifted_polyomino(variant_mask, pos);
                    if (is_placement_legal(board, border, edges, player_board, shifted))
                    {
                        all_actions.push_back(Action{
                            .type = type,
                            .variant_index = v,
                            .position = pos,
                            .shifted = shifted // optional
                        });
                    }
                }
            }
        }
    }
    return all_actions;
}
        const std::vector<Action> ALL_DISTINCT_ACTIONS =
    GetAllDistinctActions(ALL_POLYOMINO_VARIANTS, ALL_POLYOMINO_VARIANTS_INDEX);

        void update_edges(
            std::array<uint64_t, kNumBitboardParts>& edges,
            const ShiftedMask& shifted_mask
        )
        {
            std::array<uint64_t, kNumBitboardParts> piece_mask{};
            piece_mask[shifted_mask.part_index] |= shifted_mask.first_part;
            if (shifted_mask.part_index + 1 < kNumBitboardParts)
                piece_mask[shifted_mask.part_index + 1] |= shifted_mask.second_part;

            std::array<uint64_t, kNumBitboardParts> orthogonal{};

            for (int i = 0; i < kNumBitboardParts; ++i)
            {
                const uint64_t m = piece_mask[i];
                const uint64_t left = m << 1 & 0xFFFEFFFEFFFEFFFEULL;
                const uint64_t right = m >> 1 & 0x7FFF7FFF7FFF7FFFULL;
                const uint64_t lower = i > 0 ? piece_mask[i - 1] >> 48 : 0ULL;
                const uint64_t down = m << 16 | lower;
                const uint64_t upper = i < 3 ? piece_mask[i + 1] << 48 : 0ULL;
                const uint64_t up = m >> 16 | upper;
                const uint64_t up_left = up << 1 & 0xFFFEFFFEFFFEFFFEULL;
                const uint64_t up_right = up >> 1 & 0x7FFF7FFF7FFF7FFFULL;
                const uint64_t down_left = down << 1 & 0xFFFEFFFEFFFEFFFEULL;
                const uint64_t down_right = down >> 1 & 0x7FFF7FFF7FFF7FFFULL;
                orthogonal[i] |= down | up | left | right;
                edges[i] |= (up_left | up_right | down_left | down_right) & ~orthogonal[i];
            }
        }

        void place_piece(
            const Action& action, // Nimmt die Action-Struktur als konstante Referenz
            std::array<uint64_t, kNumBitboardParts>& board
        )
        {
            const ShiftedMask& mask = action.shifted;
            const int part_index = mask.part_index;
            if (part_index >= 0 && part_index < kNumBitboardParts)
            {
                board[part_index] |= mask.first_part;
            }

            if (mask.second_part != 0ULL && part_index + 1 < kNumBitboardParts)
            {
                // Wendet den Überlappungsteil auf das nächste Board-Teil an.
                board[part_index + 1] |= mask.second_part;
            }
        }

        double CalculateFinalScore(const std::array<uint64_t, kNumBitboardParts>& player_board)
        {
            double result = 0;
            for (int part_index = 0; part_index < kNumBitboardParts; part_index++)
            {
                result += __builtin_popcountll(player_board[part_index]);
            }
            return result;
        }
        std::string PolyominoTypeToString(PolyominoType type) {
            switch (type) {

            case PolyominoType::I1: return "I1";
            case PolyominoType::I2: return "I2";
            case PolyominoType::I3: return "I3";
            case PolyominoType::I4: return "I4";
            case PolyominoType::I5: return "I5";

            case PolyominoType::L3: return "L3";
            case PolyominoType::L4: return "L4";
            case PolyominoType::L5: return "L5";
            case PolyominoType::LL5: return "LL5";

            case PolyominoType::Z4: return "Z4";
            case PolyominoType::Z5: return "Z5";
            case PolyominoType::ZL5: return "ZL5";

            case PolyominoType::B4: return "B4";
            case PolyominoType::B5: return "B5";

            case PolyominoType::C5: return "C5";
            case PolyominoType::W5: return "W5";
            case PolyominoType::X5: return "X5";
            case PolyominoType::F5: return "F5";

            case PolyominoType::T4: return "T4";
            case PolyominoType::T5: return "T5";
            case PolyominoType::ST5: return "ST5";

            case PolyominoType::N0: return "N0 (Pass)";
            default: return "Unknown";
            }
        }

        std::string BoardToString(
            const std::array<uint64_t, kNumBitboardParts>& board,
            const std::array<uint64_t, kNumBitboardParts>& opponent_board,
            char player_char, char opponent_char, char empty_char)
        {
            std::string result;
            result.reserve(kNumCells + kBoardSize + 1); // 256 + 16 Zeilenumbrüche

            for (int r = 0; r < kBoardSize; ++r)
            {
                for (int c = 0; c < kBoardSize; ++c)
                {
                    int cell_index = r * kBoardSize + c;
                    int part = cell_index / 64;
                    int bit = cell_index % 64;

                    bool is_player = board[part] >> bit & 1ULL;
                    bool is_opponent = opponent_board[part] >> bit & 1ULL;

                    if (is_player)
                    {
                        result += player_char;
                    }
                    else if (is_opponent)
                    {
                        result += opponent_char;
                    }
                    else
                    {
                        result += empty_char;
                    }
                }
                result += '\n'; // Zeilenumbruch
            }
            return result;
        }

        void update_polyomino_mask(uint32_t& polyomino_mask, PolyominoType type)
        {
            const int index = static_cast<int>(type);
            polyomino_mask &= ~(1u << index);
        }

    }  // namespace blokus_duo
}  // namespace open_spiel