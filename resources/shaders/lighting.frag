#version 330 core

in vec2 v_texCoord;
in vec2 v_worldPos;
out vec4 FragColor;

uniform sampler2D u_texture;
uniform float u_ambientLight;
uniform int u_timeOfDay; // 0=Dawn, 1=Day, 2=Dusk, 3=Night
uniform int u_numPointLights;
uniform float u_exposure; // HDR exposure control
uniform bool u_useHDR; // Toggle HDR processing

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float range;
};

uniform PointLight u_pointLights[100]; // Support up to 100 lights for stress testing

// sRGB to Linear conversion
vec3 sRGBToLinear(vec3 srgb) {
    vec3 linear;
    for (int i = 0; i < 3; ++i) {
        if (srgb[i] <= 0.04045) {
            linear[i] = srgb[i] / 12.92;
        } else {
            linear[i] = pow((srgb[i] + 0.055) / 1.055, 2.4);
        }
    }
    return linear;
}

// Linear to sRGB conversion
vec3 linearToSRGB(vec3 linear) {
    vec3 srgb;
    for (int i = 0; i < 3; ++i) {
        if (linear[i] <= 0.0031308) {
            srgb[i] = linear[i] * 12.92;
        } else {
            srgb[i] = 1.055 * pow(linear[i], 1.0 / 2.4) - 0.055;
        }
    }
    return srgb;
}

// Time of day color tints
vec3 getTimeOfDayTint(int timeOfDay) {
    switch(timeOfDay) {
        case 0: return vec3(1.0, 0.8, 0.6); // Dawn - warm orange
        case 1: return vec3(1.0, 1.0, 1.0); // Day - neutral white
        case 2: return vec3(0.9, 0.7, 0.5); // Dusk - warm amber
        case 3: return vec3(0.3, 0.4, 0.7); // Night - cool blue
        default: return vec3(1.0, 1.0, 1.0);
    }
}

// Reinhard tone mapping with exposure control
vec3 toneMapReinhard(vec3 color, float exposure) {
    vec3 exposed = color * exposure;
    return exposed / (exposed + vec3(1.0));
}

// HDR tone mapping function (ACES filmic approximation)
vec3 toneMapACES(vec3 color, float exposure) {
    vec3 exposed = color * exposure;
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((exposed * (a * exposed + b)) / (exposed * (c * exposed + d) + e), 0.0, 1.0);
}

// Improved attenuation function
float calculateAttenuation(float distance, float range, float intensity) {
    // Use a smoother falloff that respects the light's range
    float normalizedDist = clamp(distance / range, 0.0, 1.0);
    float attenuation = 1.0 - normalizedDist;
    attenuation = attenuation * attenuation; // Quadratic falloff
    return attenuation * intensity;
}

void main()
{
    // Sample base texture
    vec4 baseColor = texture(u_texture, v_texCoord);

    // Convert to linear space if using HDR
    vec3 linearBaseColor = u_useHDR ? sRGBToLinear(baseColor.rgb) : baseColor.rgb;

    // Start with ambient lighting in linear space
    vec3 accumulator = linearBaseColor * u_ambientLight;

    // Light accumulation buffer for HDR
    vec3 lightAccumulation = vec3(0.0);

    // Add point light contributions
    int lightCount = min(u_numPointLights, 100);
    for(int i = 0; i < lightCount; ++i) {
        vec3 lightPos = u_pointLights[i].position;
        vec3 lightColor = u_useHDR ? sRGBToLinear(u_pointLights[i].color) : u_pointLights[i].color;
        float lightIntensity = u_pointLights[i].intensity;
        float lightRange = u_pointLights[i].range;

        // Skip lights with zero intensity
        if (lightIntensity <= 0.0001) continue;

        // Calculate distance-based attenuation
        float distance = length(lightPos.xy - v_worldPos);

        // Use improved attenuation that respects range
        float attenuation = calculateAttenuation(distance, lightRange, lightIntensity);

        // Accumulate light contribution
        if (attenuation > 0.0001) {
            vec3 lightContribution = lightColor * attenuation;
            lightAccumulation += lightContribution;
        }
    }

    // Combine base color with light accumulation
    vec3 finalColor = accumulator + (linearBaseColor * lightAccumulation);

    // Apply time of day tint
    vec3 timeOfDayTint = getTimeOfDayTint(u_timeOfDay);
    finalColor *= timeOfDayTint;

    // Apply HDR tone mapping if enabled
    if (u_useHDR) {
        // Use ACES tone mapping with exposure control
        finalColor = toneMapACES(finalColor, u_exposure);

        // Convert back to sRGB for display
        finalColor = linearToSRGB(finalColor);
    } else {
        // Simple clamping for non-HDR mode
        finalColor = clamp(finalColor, 0.0, 1.0);
    }

    FragColor = vec4(finalColor, baseColor.a);
}