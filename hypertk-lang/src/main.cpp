#include <cstdlib>
#include <iostream>
#include <string>
#include <memory>

#include "common.hpp"
#include "lexer.hpp"
#include "token.hpp"
#include "ast.hpp"
#include "parser.hpp"
#ifdef ENABLE_BUILTIN_FUNCTIONS
#include "builtin.hpp"
#endif
#ifdef ENABLE_PRINTING_AST
#include "ast_printer.hpp"
#endif
#ifndef DISABLE_SEMATIC_ANALYZING
#include "semantic_analyzer.hpp"
#endif
#include "runtime_llvm.hpp"
#include "error.hpp"

int main()
{
    std::string src = R"(
        // Logical unary not. 
        func unary!(v) {
            // return v ? 0 : 1; // There are some issue with parsing `?:`
            if (v) return 0;
            else return 1;
        }

        // Unary negate.
        func unary-(v) {
            return 0-v;
        }

        // Define > with the same precedence as <.
        func binary> 10 (LHS, RHS) {
            return RHS < LHS;
        }

        // Binary logical or, which does not short circuit.
        func binary| 5 (LHS, RHS){ 
            if (LHS)
                return 1;
            else if (RHS)
                return 1;
            else
                return 0;
        }

        // Binary logical and, which does not short circuit.
        func binary& 6 (LHS, RHS) {
            if (!LHS)
                return 0;
            else
                return !!RHS;
        }

        // Define = with slightly lower precedence than relationals.
        func binary = 9 (LHS, RHS) {
            return !(LHS < RHS | LHS > RHS);
        }

        // Define ':' for sequencing: as a low-precedence operator that ignores operands
        // and just returns the RHS.
        func binary : 1 (x, y) { return y; }

        func printdensity(d) {
            if (d > 8) 
                putchard(32);  // ' '
            else if (d > 4)
                putchard(46);  // '.'
            else if (d > 2)
                putchard(43);  // '+'
            else
                putchard(42); // '*'
        }

        // Determine whether the specific location diverges.
        // Solve for z = z^2 + c in the complex plane.
        func mandelconverger(real, imag, iters, creal, cimag) {
            if (iters > 255 | (real*real + imag*imag > 4))
                return iters;
            else
               return mandelconverger(real*real - imag*imag + creal,
                            2*real*imag + cimag,
                            iters+1, creal, cimag);
        }

        // Return the number of iterations required for the iteration to escape
        func mandelconverge(real, imag) {
            return mandelconverger(real, imag, 0, real, imag);
        }

        func mandelhelp2(xmin, xmax, xstep, y) {
            for x = xmin, x < xmax, xstep in
                printdensity(mandelconverge(x,y));
            return putchard(10);
        }

        // Compute and plot the mandelbrot set with the specified 2 dimensional range
        // info.
        func mandelhelp(xmin, xmax, xstep,   ymin, ymax, ystep) {
            for y = ymin, y < ymax, ystep in
                mandelhelp2(xmin, xmax, xstep, y);
        }

        // mandel - This is a convenient helper function for plotting the mandelbrot set
        // from the specified position with the specified Magnification.
        func mandel(realstart, imagstart, realmag, imagmag) {
            return mandelhelp(realstart, realstart+realmag*78, realmag,
                    imagstart, imagstart+imagmag*40, imagmag);
        }


        func main() {
            mandel(-2.3, -1.3, 0.05, 0.07);
            mandel(-2, -1, 0.02, 0.04);
            mandel(-0.9, -1.4, 0.02, 0.03);
            return 0;
        }
    )";
    parser::Parser parser_{lexer::Lexer{src}};

    auto ast_ = parser_.parse();
    if (error::hasError())
        return EXIT_FAILURE;
    if (ast_.has_value())
    {
#ifdef ENABLE_PRINTING_AST
        ast::SimplePrinter printer;
        printer.print(ast_.value());
#endif

        hypertk::RuntimeLLVM runtime;
#ifdef ENABLE_BASIC_JIT_COMPILER
        runtime.initializeJIT();
#else
        if (!runtime.initializeAOT())
            return EXIT_FAILURE;
#endif

        runtime.initializeModuleAndManagers();

#ifdef ENABLE_BUILTIN_FUNCTIONS
        runtime.declareBuiltInFunctions();
#endif

#ifndef DISABLE_SEMATIC_ANALYZING
        semantic_analysis::BasicSemanticAnalyzer analyzer(ast_.value());
        if (!analyzer.analyze())
            return EXIT_FAILURE;
#endif

        runtime.genIR(ast_.value());
        if (error::hasError())
            return EXIT_FAILURE;
#ifdef ENABLE_PRINTING_LLVM_IR
        runtime.printIR();
#endif

#ifdef ENABLE_BASIC_JIT_COMPILER
        runtime.eval();
#else
        if (!runtime.compileToObjectFile("output.o"))
            return EXIT_FAILURE;
#endif
    }

    return EXIT_SUCCESS;
}