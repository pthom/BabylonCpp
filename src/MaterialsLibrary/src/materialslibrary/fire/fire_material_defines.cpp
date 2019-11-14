#include <babylon/materialslibrary/fire/fire_material_defines.h>

namespace BABYLON {
namespace MaterialsLibrary {

FireMaterialDefines::FireMaterialDefines() : MaterialDefines{}
{
  boolDef = {
    {"DIFFUSE", false},      //
    {"CLIPPLANE", false},    //
    {"CLIPPLANE2", false},   //
    {"CLIPPLANE3", false},   //
    {"CLIPPLANE4", false},   //
    {"ALPHATEST", false},    //
    {"DEPTHPREPASS", false}, //
    {"POINTSIZE", false},    //
    {"FOG", false},          //
    {"UV1", false},          //
    {"VERTEXCOLOR", false},  //
    {"VERTEXALPHA", false},  //
    {"INSTANCES", false},    //
  };

  intDef = {
    {"NUM_BONE_INFLUENCERS", 0}, //
    {"BonesPerMesh", 0}          //
  };
}

FireMaterialDefines::~FireMaterialDefines() = default;

} // end of namespace MaterialsLibrary
} // end of namespace BABYLON
