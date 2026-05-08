#ifndef CELL_H
#define CELL_H

#include <ast.h>

struct Cell {

    public:
        Cell(){}

        /// Place genome into cell if it is empty, or if this genome has higher quality. Returns one if cell was
        /// empty, so we can track unique archive placements 
        inline bool place(const Ast_entry& new_genome, float genome_quality){
            bool new_placement = false;

            if (genome.empty() || (quality < genome_quality)){
                if (genome.empty()){
                    // completely new placement into the archive
                    new_placement = true;
                }
                
                genome = new_genome;
                quality = genome_quality;
            }

            return new_placement;
        }

        inline float get_quality() const {
            return quality;
        }

        inline bool empty() const {
            return genome.empty();
        }

        inline Ast_entry get_genome() const {
            if (genome.get_ast() == nullptr){
                ERROR("Genome root is nullptr");
            }

            return genome;
        }

    private:
        Ast_entry genome;
        float quality = 0.0;
};


#endif
