#ifndef __OVXLIB_TRANSFORMATIONS_H__
#define __OVXLIB_TRANSFORMATIONS_H__

#include <vector>
#include <set>

#include "nnrt/model.hpp"
#include "nnrt/op/public.hpp"

namespace nnrt
{
class Transformation
{
    public:
        virtual ~Transformation(){};

        virtual int run(Model * model, bool * modified) = 0;

        virtual const char* name() = 0;
};
#define NEW_GRAPH_TRANSFORMATION(Name)                      \
    class Name : public Transformation                      \
    {                                                       \
        public:                                             \
            int run(Model* model,                           \
                    bool* modified) override;               \
            const char* name() override {return #Name;}     \
    }

class TransformationSet
{
    public:
        virtual ~TransformationSet();
        virtual void add(Transformation* transformation);
        virtual int run(Model* model);
        virtual int once(Model* model);

    private:
        std::vector<Transformation*> transformations_;
};

NEW_GRAPH_TRANSFORMATION(LayoutInference);
NEW_GRAPH_TRANSFORMATION(OptimizePermute);
NEW_GRAPH_TRANSFORMATION(Fp32ToFp16);
NEW_GRAPH_TRANSFORMATION(ValidateQuantizedGraph);
NEW_GRAPH_TRANSFORMATION(AlignBroadcastOp);

#undef NEW_GRAPH_TRANSFORMATION
}
#endif
