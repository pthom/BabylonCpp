#include <babylon/materialslibrary/triplanar/tri_planar_material_defines.h>

namespace BABYLON {
namespace MaterialsLibrary {

TriPlanarMaterialDefines::TriPlanarMaterialDefines() : MaterialDefines{}
{
  boolDef = {
    {"DIFFUSEX", false}, //
    {"DIFFUSEY", false}, //
    {"DIFFUSEZ", false}, //

    {"BUMPX", false}, //
    {"BUMPY", false}, //
    {"BUMPZ", false}, //

    {"CLIPPLANE", false},    //
    {"CLIPPLANE2", false},   //
    {"CLIPPLANE3", false},   //
    {"CLIPPLANE4", false},   //
    {"ALPHATEST", false},    //
    {"DEPTHPREPASS", false}, //
    {"POINTSIZE", false},    //
    {"FOG", false},          //
    {"SPECULARTERM", false}, //
    {"NORMAL", false},       //
    {"VERTEXCOLOR", false},  //
    {"VERTEXALPHA", false},  //
    {"INSTANCES", false},    //
  };

  intDef = {
    {"NUM_BONE_INFLUENCERS", 0}, //
    {"BonesPerMesh", 0}          //
  };
}

TriPlanarMaterialDefines::~TriPlanarMaterialDefines() = default;

} // end of namespace MaterialsLibrary
} // end of namespace BABYLON
