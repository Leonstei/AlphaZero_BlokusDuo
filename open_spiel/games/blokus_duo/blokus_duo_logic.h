//
// Created by leoney on 05.11.25.
//

#ifndef OPEN_SPIEL_BLOKUS_DUO_LOGIC_H
#define OPEN_SPIEL_BLOKUS_DUO_LOGIC_H

#include <array>
#include <unordered_map>
#include <vector>



namespace open_spiel {
    namespace blokus_duo {

        enum class PolyominoType {
            I1, I2, I3, I4, I5,
            L3, L4, L5, LL5, //LL = Langes L mit langezogenem unteren strich
            Z4, Z5, ZL5,
            B4, B5, //Block
            C5, W5, X5, F5,   // Fünfer
            T4, T5, ST5, // ST = Small T
            N0 // Null
        };


        using PolyominoBitboards = std::vector<uint64_t>;
        using PolyominoMap = std::unordered_map<PolyominoType, PolyominoBitboards>;
        using PolyominoIndexMap = std::unordered_map<PolyominoType, std::vector<std::vector<int>>>;

        const PolyominoIndexMap ALL_POLYOMINO_VARIANTS_INDEX = {
        {PolyominoType::I1,
            {{0},}},
        { PolyominoType::I2,
            {{ 0, 1,},
                { 0, 16,} } },
        {PolyominoType::I3,
            { { 0, 1, 2,},
                { 0, 16, 32, } } },
        {PolyominoType::I4,
            { { 0, 1, 2, 3,  },
                { 0, 16, 32, 48,  } }
        },{PolyominoType::I5,
            {{ 0, 1, 2, 3, 4, },
                { 0, 16, 32, 48, 64,} }
        },{PolyominoType::L3,
    {{ 0, 1, 16, },
        { 0, 1, 17, },
        {  1, 16, 17, },
        { 0, 16, 17,} }
        },{ PolyominoType::L4,
            { {   0, 16, 32, 33, },
                { 1, 17, 32, 33,  },
                { 0, 1, 2, 16, },
                { 0, 1, 2, 18,},
                { 0, 1, 17, 33,},
                { 0, 1, 16, 32, },
                { 2, 16, 17, 18, },
                { 0, 16, 17, 18,},}
        },{ PolyominoType::L5,
        { {  0, 16, 32, 48, 49, },
            { 1, 17, 33, 48, 49,  },
            { 0, 1, 2, 3, 16, },
            { 0, 1, 2, 3, 19,},
            { 0, 1, 17, 33, 49,},
            { 0, 1, 16, 32, 48, },
            { 3, 16, 17, 18, 19, },
            { 0, 16, 17, 18, 19, },}
        },{ PolyominoType::LL5,
        { { 0, 16, 32, 33, 34,  },
            { 2, 18, 32, 33, 34,  },
            { 0, 1, 2, 16, 32, },
            { 0, 1, 2, 18, 34, },}
        },{ PolyominoType::B4,
        { {  0, 1, 16, 17,  },}
        },{ PolyominoType::B5,
        { {  0, 1, 16, 17, 33, },
            { 0, 1, 16, 17, 32,   },
            { 1, 2, 16, 17, 18, },
            { 0, 1, 16, 17, 18, },
            { 0, 16, 17, 32, 33,},
            { 1, 16, 17, 32, 33,},
            { 0, 1, 2, 16, 17,  },
            { 0, 1, 2, 17, 18, },}
        },{ PolyominoType::C5,
        { { 0, 1, 2, 16, 18,  },
            { 0, 1, 17, 32, 33,   },
            { 0, 1, 16, 32, 33, },
            { 0, 2, 16, 17, 18,},}
        },{ PolyominoType::W5,
        { { 0, 16, 17, 33, 34, },
            { 2, 17, 18, 32, 33,   },
            { 1, 2, 16, 17, 32,  },
            { 0, 1, 17, 18, 34, },}
        },{ PolyominoType::X5,
        { {  1, 16, 17, 18, 33,  },}
        },{ PolyominoType::F5,
        { {  0, 16, 17, 18, 33, },
            { 2, 16, 17, 18, 33,   },
            { 1, 2, 16, 17, 33, },
            { 0, 1, 17, 18, 33, },
            { 1, 16, 17, 18, 34, },
            { 1, 16, 17, 18, 32, },
            { 1, 17, 18, 32, 33, },
            { 1, 16, 17, 33, 34, },}
        },{ PolyominoType::T4,
        { {  0, 16, 17, 32,  },
            { 1, 16, 17, 33,   },
            { 0, 1, 2, 17, },
            { 1, 16, 17, 18, },}
        },{ PolyominoType::T5,
        { { 0, 1, 2, 17, 33, },
            { 2, 16, 17, 18, 34,   },
            { 0, 16, 17, 18, 32,  },
            { 1, 17, 32, 33, 34, },}
        },{ PolyominoType::ST5,
        { { 0, 16, 17, 32, 48,  },
            { 1, 16, 17, 33, 49,   },
            { 0, 1, 2, 3, 18,  },
            { 0, 1, 2, 3, 17, },
            { 1, 17, 32, 33, 49, },
            { 0, 16, 32, 33, 48,  },
            { 1, 16, 17, 18, 19, },
            { 2, 16, 17, 18, 19, },}
        },{ PolyominoType::Z4,
        { {  0, 16, 17, 33,  },
            { 1, 16, 17, 32,   },
            { 1, 2, 16, 17,  },
            { 0, 1, 17, 18,  },}
        },{ PolyominoType::Z5,
        { {  0, 16, 17, 18, 34,  },
            { 2, 16, 17, 18, 32,   },
            { 1, 2, 17, 32, 33,   },
            { 0, 1, 17, 33, 34, },}
        },{ PolyominoType::ZL5,
        { {  0, 16, 32, 33, 49,  },
            { 1, 17, 32, 33, 48,  },
            { 1, 2, 3, 16, 17, },
            { 0, 1, 2, 18, 19, },
            { 0, 16, 17, 33, 49, },
            { 1, 16, 17, 32, 48, },
            { 2, 3, 16, 17, 18,  },
            { 0, 1, 17, 18, 19,},}
        },

    };

const  std::unordered_map<PolyominoType, std::vector<uint64_t>> ALL_POLYOMINO_VARIANTS = {
        {PolyominoType::I1,{
            1,
        }},{PolyominoType::I2, {
            3, 65537
        }},{PolyominoType::I3, {
            7, 4295032833
        }},{PolyominoType::I4, {
            15, 281479271743489
        }},
        {PolyominoType::I5, {
            31, 255 //hier ändern
         }},
        {            PolyominoType::L3, {
            65539, 131075, 196610, 196609
        }},{PolyominoType::L4, {
            12884967425, 12885032962, 65543, 262151, 8590065667, 4295032835, 458756, 458753
        }},{PolyominoType::L5, {
            844429225164801, 844433520197634, 65551, 524303, 562958543486979, 281479271743491, 983048, 983041
        }},{PolyominoType::LL5, {
            30064836609, 30065033220, 4295032839, 17180131335
        }},{PolyominoType::B4, {
            196611
        }},{PolyominoType::B5, {
            8590131203 ,4295163907, 458758, 458755, 12885098497, 12885098498, 196615, 393223
        }},{PolyominoType::C5, {
            327687, 12885032963, 12884967427, 458757
        }},{PolyominoType::W5, {
            25770000385, 12885295108, 4295163910, 17180262403
        }},{PolyominoType::X5, {
            8590393346
        }},{PolyominoType::F5, {
            8590393345, 8590393348, 8590131206, 8590327811, 17180327938, 4295426050, 12885295106, 25770000386
        }},{PolyominoType::T4, {
            4295163905, 8590131202, 131079, 458754
        }},{PolyominoType::T5, {
            8590065671, 17180327940, 4295426049, 30064902146
        }},{PolyominoType::ST5, {
            281479271874561, 562958543552514, 262159, 131087, 562962838454274, 281487861678081, 983042, 983044
        }},{PolyominoType::Z4, {
            8590131201, 4295163906, 196614, 393219
        }},{PolyominoType::Z5, {
            17180327937 ,4295426052, 12885032966, 25769934851
        }},{PolyominoType::ZL5, {
            562962838388737 ,281487861743618, 196622, 786439, 562958543552513, 281479271874562, 458764, 917507
        }},
    };
        // Constants.
        inline constexpr int kNumPlayers = 2;
        inline constexpr int kBoardSize = 16;
        inline constexpr int kBoardSizeWithoutBorder = 14;
        inline constexpr int kNumCells = kBoardSize * kBoardSize;
        inline constexpr int kNumBitboardParts = kNumCells / 64;
        inline constexpr int kNumPolyominoTypes = 21;
        inline constexpr int max_game_length = 42;
        inline constexpr int kNumBlokusDuoPlacementActions = 13729;
        constexpr int kPassAction = kNumBlokusDuoPlacementActions; // 13729
        constexpr int kNumDistinctActions = kNumBlokusDuoPlacementActions + 1;
        constexpr int kNumBoardChannels = 4; // Steine und Ecken
        constexpr int kNumGlobalChannels = 2 * kNumPolyominoTypes; // 2 * 21 = 42
        constexpr int kTotalChannels = kNumBoardChannels + kNumGlobalChannels;

        inline std::array<uint64_t, kNumBitboardParts> CreateCenterMask() {
            std::array<uint64_t, kNumBitboardParts> mask{}; // Initialisiert mit Nullen

            // Wir definieren die 8x8-Region von Row 4 bis Row 11 und Col 4 bis Col 11.
            for (int row = 4; row < 12; ++row) {
                for (int col = 4; col < 12; ++col) {
                    int index = row * kBoardSize + col; // 0 bis 255

                    // Berechne Teil-Index (Part) und Bit-Index innerhalb des Teils
                    int part_index = index / 64;
                    int bit_index = index % 64;

                    if (part_index < kNumBitboardParts) {
                        mask[part_index] |= (1ULL << bit_index);
                    }
                }
            }
            return mask;
        }
        const std::array<uint64_t, kNumBitboardParts> kCenterMask = CreateCenterMask();
        // inline constexpr int max_game_length = 42;
        // inline constexpr int max_game_length = 42;

        inline bool IsBitSet(const std::array<uint64_t, kNumBitboardParts>& board,int r, int c) {
            const int index = r * kBoardSize + c;
            int part = index / 64;
            int bit = index % 64;
            return (board[part] & (1ULL << bit)) != 0;
        }


        struct ShiftedMask {
            uint64_t first_part = 0ULL;
            uint64_t second_part = 0ULL;
            int part_index = -1;
        };
        struct Action {
            PolyominoType type = PolyominoType::N0;
            int variant_index = -1;
            int position = -1;
            ShiftedMask shifted = {};
        };
        extern const std::vector<Action> ALL_DISTINCT_ACTIONS;

        ShiftedMask shifted_polyomino(uint64_t polyomino, int pos);
        bool HasPolyomino(PolyominoType type, uint32_t polyomino_mask);

        double CalculateFinalScore(const std::array<uint64_t, kNumBitboardParts>& player_board);

        bool is_colliding_with_border(const ShiftedMask& shifted_mask,const std::array<uint64_t, kNumBitboardParts>& border);
        bool is_colliding(const ShiftedMask& shifted_mask,const std::array<uint64_t, kNumBitboardParts>& board);
        bool is_in_edges(
            const ShiftedMask& shifted_mask,
            const std::array<uint64_t, kNumBitboardParts>& edges
        );
        bool is_touching_own_polyominos(
            const ShiftedMask& shifted_mask,
            const std::array<uint64_t, kNumBitboardParts>& own_board
        );
        bool is_placement_legal(const std::array<uint64_t, kNumBitboardParts>& board,
                                const std::array<uint64_t, kNumBitboardParts>& border,
                                const std::array<uint64_t, kNumBitboardParts>& edges,
                                const std::array<uint64_t, kNumBitboardParts>& own_board,
                                const ShiftedMask& shifted_mask);
        void place_piece(
            const Action& action,
            std::array<uint64_t, kNumBitboardParts>& board
        );
        void update_edges(
            const std::array<uint64_t, kNumBitboardParts>& board,
            const std::array<uint64_t, kNumBitboardParts>& opponent_board,
            std::array<uint64_t, kNumBitboardParts>& edges,
            std::array<uint64_t, kNumBitboardParts>& opponent_edges
        );
        void calculate_edges(
            const std::array<uint64_t, kNumBitboardParts>& combined_board,
            const std::array<uint64_t, kNumBitboardParts>& current_palyer_board,
            const std::array<uint64_t, kNumBitboardParts>& opponent_board,
            std::array<uint64_t, kNumBitboardParts>& edges,
            std::array<uint64_t, kNumBitboardParts>& opponent_edges
            );
        void printboard(std::array<uint64_t, kNumBitboardParts>& board);
        std::vector<int> edges_to_indices(const std::array<uint64_t, kNumBitboardParts>& edges);
        int count_edges(const std::array<uint64_t, kNumBitboardParts>& edges);
        double legal_moves_difference(
        const std::array<uint64_t, kNumBitboardParts>& combined_board_,
        const std::array<uint64_t, kNumBitboardParts>& border,
        const std::array<uint64_t, kNumBitboardParts>& current_player_board,
        const std::array<uint64_t, kNumBitboardParts>& opponent_board,
        const std::array<uint64_t, kNumBitboardParts>& current_player_edges,
        const std::array<uint64_t, kNumBitboardParts>& opponent_edges,
        uint32_t polyomino_mask_currentplayer,
        uint32_t polyomino_mask_opponent
            );
        double evaluateCenterControll(
            const std::array<uint64_t, kNumBitboardParts>& current_player_board,
            const std::array<uint64_t, kNumBitboardParts>& opponent_board);
        std::vector<int> polyomino_to_indices(const uint64_t polyomino);

        std::vector<Action> LegalActions(const std::array<uint64_t, kNumBitboardParts>& board,
                                         const std::array<uint64_t, kNumBitboardParts>& player_board,
                                         const std::array<uint64_t, kNumBitboardParts>& border,
                                         const std::array<uint64_t, kNumBitboardParts>& edges,
                                         const std::unordered_map<PolyominoType, PolyominoBitboards>& all_polyomino_variants,
                                         const std::unordered_map<PolyominoType, std::vector<std::vector<int>>>& all_polyomino_indexes,
                                         uint32_t polyomino_mask);

        std::string PolyominoTypeToString(PolyominoType type);
        std::string BoardToString(
            const std::array<uint64_t, kNumBitboardParts>& board,
            const std::array<uint64_t, kNumBitboardParts>& opponent_board,
            char player_char, char opponent_char, char empty_char = '.');
        void update_polyomino_mask(uint32_t& polyomino_mask, PolyominoType type);
        void add_polyomino_to_mask(uint32_t& polyomino_mask, PolyominoType type);



    }  // namespace blokus_duo
}  // namespace open_spiel

#endif //OPEN_SPIEL_BLOKUS_DUO_LOGIC_H