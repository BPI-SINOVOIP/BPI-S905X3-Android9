/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.databinding.tool.expr;

import android.databinding.tool.reflection.ModelAnalyzer;
import android.databinding.tool.reflection.ModelClass;
import android.databinding.tool.solver.ExecutionPath;
import android.databinding.tool.writer.KCode;

import java.util.ArrayList;
import java.util.List;

public class SymbolExpr extends Expr {
    String mText;
    Class mType;

    SymbolExpr(String text, Class type) {
        super();
        mText = text;
        mType = type;
    }

    public String getText() {
        return mText;
    }

    @Override
    protected ModelClass resolveType(ModelAnalyzer modelAnalyzer) {
        return modelAnalyzer.findClass(mType);
    }

    @Override
    protected String computeUniqueKey() {
        return mType.getSimpleName() + mText;
    }

    @Override
    public String getInvertibleError() {
        return "Symbol '" + mText + "' cannot be the target of a two-way binding expression";
    }

    @Override
    protected KCode generateCode() {
        return new KCode(getText());
    }

    @Override
    public Expr cloneToModel(ExprModel model) {
        return model.symbol(mText, mType);
    }

    @Override
    protected List<Dependency> constructDependencies() {
        return new ArrayList<Dependency>();
    }

    @Override
    public boolean canBeEvaluatedToAVariable() {
        return !void.class.equals(mType);
    }

    @Override
    public List<ExecutionPath> toExecutionPath(List<ExecutionPath> paths) {
        if (void.class.equals(mType)) {
            return paths;
        }
        return super.toExecutionPath(paths);
    }

    @Override
    public String toString() {
        return mText;
    }
}
