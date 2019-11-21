﻿#ifndef BABYLON_SHADERS_DEPTH_FRAGMENT_FX_H
#define BABYLON_SHADERS_DEPTH_FRAGMENT_FX_H

namespace BABYLON {

extern const char* depthPixelShader;

const char* depthPixelShader
  = R"ShaderCode(

#ifdef ALPHATEST
varying vec2 vUV;
uniform sampler2D diffuseSampler;
#endif

varying float vDepthMetric;

void main(void)
{
#ifdef ALPHATEST
    if (texture2D(diffuseSampler, vUV).a < 0.4)
        discard;
#endif

    gl_FragColor = vec4(vDepthMetric, vDepthMetric * vDepthMetric, 0.0, 1.0);
}

)ShaderCode";
} // end of namespace BABYLON

#endif // end of BABYLON_SHADERS_DEPTH_FRAGMENT_FX_H
