// Copyright Chad Engler

#include "input_text.h"

namespace he::editor
{
    struct InputTextCallback_UserData
    {
        String* str;
        ImGuiInputTextCallback chainCallback;
        void* chainCallbackUserData;
    };

    static int InputTextCallback(ImGuiInputTextCallbackData* data)
    {
        InputTextCallback_UserData* userData = (InputTextCallback_UserData*)data->UserData;
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
        {
            // Resize string callback
            // If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
            String* str = userData->str;
            IM_ASSERT(data->Buf == str->Data());
            str->Resize(data->BufTextLen);
            data->Buf = str->Data();
        }
        else if (userData->chainCallback)
        {
            // Forward to user callback, if any
            data->UserData = userData->chainCallbackUserData;
            return userData->chainCallback(data);
        }
        return 0;
    }

    bool InputText(const char* label, String& str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* userData)
    {
        IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
        flags |= ImGuiInputTextFlags_CallbackResize;

        InputTextCallback_UserData cbUserData;
        cbUserData.str = &str;
        cbUserData.chainCallback = callback;
        cbUserData.chainCallbackUserData = userData;
        return ImGui::InputText(label, str.Data(), str.Capacity() + 1, flags, InputTextCallback, &cbUserData);
    }
}
