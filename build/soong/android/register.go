// Copyright 2015 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package android

import (
	"github.com/google/blueprint"
)

type moduleType struct {
	name    string
	factory blueprint.ModuleFactory
}

var moduleTypes []moduleType

type singleton struct {
	name    string
	factory blueprint.SingletonFactory
}

var singletons []singleton
var preSingletons []singleton

type mutator struct {
	name            string
	bottomUpMutator blueprint.BottomUpMutator
	topDownMutator  blueprint.TopDownMutator
	parallel        bool
}

var mutators []*mutator

type ModuleFactory func() Module

// ModuleFactoryAdaptor wraps a ModuleFactory into a blueprint.ModuleFactory by converting a Module
// into a blueprint.Module and a list of property structs
func ModuleFactoryAdaptor(factory ModuleFactory) blueprint.ModuleFactory {
	return func() (blueprint.Module, []interface{}) {
		module := factory()
		return module, module.GetProperties()
	}
}

type SingletonFactory func() Singleton

// SingletonFactoryAdaptor wraps a SingletonFactory into a blueprint.SingletonFactory by converting
// a Singleton into a blueprint.Singleton
func SingletonFactoryAdaptor(factory SingletonFactory) blueprint.SingletonFactory {
	return func() blueprint.Singleton {
		singleton := factory()
		return singletonAdaptor{singleton}
	}
}

func RegisterModuleType(name string, factory ModuleFactory) {
	moduleTypes = append(moduleTypes, moduleType{name, ModuleFactoryAdaptor(factory)})
}

func RegisterSingletonType(name string, factory SingletonFactory) {
	singletons = append(singletons, singleton{name, SingletonFactoryAdaptor(factory)})
}

func RegisterPreSingletonType(name string, factory SingletonFactory) {
	preSingletons = append(preSingletons, singleton{name, SingletonFactoryAdaptor(factory)})
}

type Context struct {
	*blueprint.Context
}

func NewContext() *Context {
	return &Context{blueprint.NewContext()}
}

func (ctx *Context) Register() {
	for _, t := range preSingletons {
		ctx.RegisterPreSingletonType(t.name, t.factory)
	}

	for _, t := range moduleTypes {
		ctx.RegisterModuleType(t.name, t.factory)
	}

	for _, t := range singletons {
		ctx.RegisterSingletonType(t.name, t.factory)
	}

	registerMutators(ctx.Context, preArch, preDeps, postDeps)

	ctx.RegisterSingletonType("env", SingletonFactoryAdaptor(EnvSingleton))
}
