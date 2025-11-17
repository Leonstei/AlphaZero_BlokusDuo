//
// Created by leoney on 05.11.25.
//

#ifndef OPEN_SPIEL_POLYOMINO_DATA_H
#define OPEN_SPIEL_POLYOMINO_DATA_H


#include <vector>
#include <unordered_map>

namespace open_spiel{
    namespace blokus_duo{

        // enum class PolyominoType {
        //     I1, I2, I3, I4, I5,
        //     L3, L4, L5, LL5, //LL = Langes L mit langezogenem unteren strich
        //     Z4, Z5, ZL5,
        //     B4, B5, //Block
        //     C5, W5, X5, F5,   // Fünfer
        //     T4, T5, ST5, // ST = Small T
        //     N0 // Null
        // };


        // using PolyominoBitboards = std::vector<uint64_t>;
        // using PolyominoMap = std::unordered_map<PolyominoType, PolyominoBitboards>;
        // using PolyominoIndexMap = std::unordered_map<PolyominoType, std::vector<std::vector<int>>>;


        // //extern const PolyominoMap ALL_POLYOMINO_VARIANTS;
        // extern const PolyominoIndexMap ALL_POLYOMINO_VARIANTS_INDEX;

    }
}

#endif //OPEN_SPIEL_POLYOMINO_DATA_H