#version 330 core

layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_texCoord;

uniform mat4 u_projection;
uniform mat4 u_view;
uniform mat4 u_model;

out vec2 v_texCoord;
out vec2 v_worldPos;

void main()
{
    vec4 worldPos = u_model * vec4(a_position, 0.0, 1.0);
    gl_Position = u_projection * u_view * worldPos;

    v_texCoord = a_texCoord;
    v_worldPos = worldPos.xy;
}