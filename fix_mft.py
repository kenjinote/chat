import re

with open('H264Helper.cpp', 'r', encoding='utf-8') as f:
    code = f.read()

def replace_mft_alloc(match):
    prefix = match.group(1)
    sample_var = match.group(2)
    buffer_var = match.group(3)
    size_expr = match.group(4)
    output_var = match.group(5)
    
    mft_var = ""
    if sample_var == "pConvSample":
        mft_var = "pConverter"
    elif sample_var == "pEncSample":
        mft_var = "pEncoder"
    elif sample_var == "pOutSample":
        mft_var = "pDecoder"
    
    new_code = f"""{prefix}MFT_OUTPUT_STREAM_INFO info = {{0}};
{prefix}{mft_var}->GetOutputStreamInfo(0, &info);
{prefix}IMFSample* {sample_var} = NULL;
{prefix}if ((info.dwFlags & (MFT_OUTPUT_STREAM_PROVIDES_SAMPLES | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES)) == 0) {{
{prefix}    MFCreateSample(&{sample_var});
{prefix}    IMFMediaBuffer* {buffer_var} = NULL;
{prefix}    MFCreateMemoryBuffer({size_expr}, &{buffer_var});
{prefix}    {sample_var}->AddBuffer({buffer_var});
{prefix}    {buffer_var}->Release();
{prefix}    {output_var}.pSample = {sample_var};
{prefix}}} else {{
{prefix}    {output_var}.pSample = NULL;
{prefix}}}"""
    return new_code

code = re.sub(r'([ \t]+)IMFSample\*\s+([a-zA-Z0-9_]+)\s*=\s*NULL;\s*MFCreateSample\(&\2\);\s*IMFMediaBuffer\*\s+([a-zA-Z0-9_]+)\s*=\s*NULL;\s*MFCreateMemoryBuffer\(([^,]+),\s*&\3\);\s*\2->AddBuffer\(\3\);\s*\3->Release\(\);\s*([a-zA-Z0-9_]+)\.pSample\s*=\s*\2;', 
              replace_mft_alloc, code)

def fix_cleanup(code):
    code = code.replace(
        "if (encOutput.pSample) encOutput.pSample->Release();\n                        pEncSample->Release();",
        "if (encOutput.pSample) encOutput.pSample->Release();"
    )
    code = code.replace(
        "if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {\n                        pEncSample->Release();\n                        break;\n                    }",
        "if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {\n                        if (pEncSample) pEncSample->Release();\n                        break;\n                    }"
    )
    code = code.replace(
        "if (convOutput.pSample) convOutput.pSample->Release();\n            pConvSample->Release();",
        "if (convOutput.pSample) convOutput.pSample->Release();"
    )
    code = code.replace(
        "if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {\n            pConvSample->Release();\n            break;\n        }",
        "if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {\n            if (pConvSample) pConvSample->Release();\n            break;\n        }"
    )
    code = code.replace(
        "if (outputDataBuffer.pSample) outputDataBuffer.pSample->Release();\n            pOutSample->Release();",
        "if (outputDataBuffer.pSample) outputDataBuffer.pSample->Release();"
    )
    code = code.replace(
        "if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {\n            pOutSample->Release();\n            break;\n        }",
        "if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {\n            if (pOutSample) pOutSample->Release();\n            break;\n        }"
    )
    code = code.replace(
        "if (convOutput.pSample) convOutput.pSample->Release();\n                        pConvSample->Release();",
        "if (convOutput.pSample) convOutput.pSample->Release();"
    )
    code = code.replace(
        "if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {\n                        pConvSample->Release();\n                        break;\n                    }",
        "if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {\n                        if (pConvSample) pConvSample->Release();\n                        break;\n                    }"
    )
    
    return code

code = fix_cleanup(code)

with open('H264Helper.cpp', 'w', encoding='utf-8') as f:
    f.write(code)

print("Done")
