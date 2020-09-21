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
import android.databinding.tool.writer.KCode;

import java.util.List;

public class CastExpr extends Expr {

    final String mType;

    CastExpr(String type, Expr expr) {
        super(expr);
        mType = type;
    }

    @Override
    protected ModelClass resolveType(ModelAnalyzer modelAnalyzer) {
        return modelAnalyzer.findClass(mType, getModel().getImports());
    }

    @Override
    protected List<Dependency> constructDependencies() {
        final List<Dependency> dependencies = constructDynamicChildrenDependencies();
        for (Dependency dependency : dependencies) {
            dependency.setMandatory(true);
        }
        return dependencies;
    }

    protected String computeUniqueKey() {
        return join(mType, getCastExpr().computeUniqueKey());
    }

    public Expr getCastExpr() {
        return getChildren().get(0);
    }

    public String getCastType() {
        return getResolvedType().toJavaCode();
    }

    @Override
    protected KCode generateCode() {
        return new KCode()
                .app("(")
                .app(getCastType())
                .app(") (", getCastExpr().toCode())
                .app(")");
    }

    @Override
    public String getInvertibleError() {
        return getCastExpr().getInvertibleError();
    }

    @Override
    public Expr generateInverse(ExprModel model, Expr value, String bindingClassName) {
        Expr castExpr = getCastExpr();
        ModelClass exprType = castExpr.getResolvedType();
        Expr castValue = model.castExpr(exprType.toJavaCode(), value);
        return castExpr.generateInverse(model, castValue, bindingClassName);
    }

    @Override
    public Expr cloneToModel(ExprModel model) {
        return model.castExpr(mType, getCastExpr().cloneToModel(model));
    }

    @Override
    public String toString() {
        return "(" + mType + ") " + getCastExpr();
    }
}
