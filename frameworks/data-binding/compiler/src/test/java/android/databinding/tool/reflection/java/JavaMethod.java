/*
 * Copyright (C) 2015 The Android Open Source Project
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *      http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.databinding.tool.reflection.java;


import android.databinding.Bindable;
import android.databinding.tool.reflection.ModelClass;
import android.databinding.tool.reflection.ModelMethod;
import android.databinding.tool.reflection.SdkUtil;
import android.databinding.tool.reflection.TypeUtil;

import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.List;

public class JavaMethod extends ModelMethod {
    public final Method mMethod;

    public JavaMethod(Method method) {
        mMethod = method;
    }


    @Override
    public ModelClass getDeclaringClass() {
        return new JavaClass(mMethod.getDeclaringClass());
    }

    @Override
    public ModelClass[] getParameterTypes() {
        Class[] parameterTypes = mMethod.getParameterTypes();
        ModelClass[] parameterClasses = new ModelClass[parameterTypes.length];
        for (int i = 0; i < parameterTypes.length; i++) {
            parameterClasses[i] = new JavaClass(parameterTypes[i]);
        }
        return parameterClasses;
    }

    @Override
    public String getName() {
        return mMethod.getName();
    }

    @Override
    public ModelClass getReturnType(List<ModelClass> args) {
        return new JavaClass(mMethod.getReturnType());
    }

    @Override
    public boolean isVoid() {
        return void.class.equals(mMethod.getReturnType());
    }

    @Override
    public boolean isPublic() {
        return Modifier.isPublic(mMethod.getModifiers());
    }

    @Override
    public boolean isProtected() {
        return Modifier.isProtected(mMethod.getModifiers());
    }

    @Override
    public boolean isStatic() {
        return Modifier.isStatic(mMethod.getModifiers());
    }

    @Override
    public boolean isAbstract() {
        return Modifier.isAbstract(mMethod.getModifiers());
    }

    @Override
    public boolean isBindable() {
        return mMethod.getAnnotation(Bindable.class) != null;
    }

    @Override
    public int getMinApi() {
        return SdkUtil.getMinApi(this);
    }

    @Override
    public String getJniDescription() {
        return TypeUtil.getInstance().getDescription(this);
    }

    @Override
    public boolean isVarArgs() {
        return false;
    }
}
