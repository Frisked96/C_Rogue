
"""
WHEN WORKING IN A TERMINAL ENVIRONMENT.
cppcheck.py - Fast C++ syntax checker similar to cargo check
Usage: python cppcheck.py [path] [--flags FLAGS]
"""

import os
import sys
import subprocess
import argparse
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor, as_completed
from typing import List, Tuple

class CppSyntaxChecker:
    def __init__(self, clang_path="clang++", flags="-std=c++17 -Wall -Wextra"):
        self.clang = clang_path
        self.flags = flags
        self.errors = []
    
    def check_file(self, filepath: Path) -> Tuple[bool, str]:
        """Check a single C++ file for syntax errors"""
        try:
            cmd = [self.clang, "-fsyntax-only", *self.flags.split(), str(filepath)]
            
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=10  # Timeout after 10 seconds per file
            )
            
            if result.returncode == 0:
                return (True, f"✅ {filepath.name}")
            else:
                error_msg = result.stderr.strip() or result.stdout.strip()
                return (False, f"❌ {filepath.name}\n   {error_msg[:200]}...")
                
        except subprocess.TimeoutExpired:
            return (False, f"⏱️  {filepath.name} (timeout)")
        except Exception as e:
            return (False, f"⚠️  {filepath.name} (error: {str(e)})")
    
    def find_cpp_files(self, root_dir: Path) -> List[Path]:
        """Find all .cpp, .cc, .h, .hpp files recursively"""
        extensions = {'.cpp', '.cc', '.cxx', '.h', '.hpp', '.hh', '.hxx'}
        cpp_files = []
        
        for ext in extensions:
            cpp_files.extend(root_dir.rglob(f"*{ext}"))
        
        # Filter out build directories
        filtered = []
        for file in cpp_files:
            # Skip common build directories
            if any(part.startswith('.') or part in {'build', 'cmake-build', 'node_modules'} 
                   for part in file.parts):
                continue
            filtered.append(file)
        
        return filtered
    
    def run(self, path: str, workers: int = 4):
        """Main checking function"""
        root = Path(path).resolve()
        
        if not root.exists():
            print(f"Error: Path '{path}' does not exist")
            return False
        
        print(f"🔍 Checking C++ files in: {root}")
        print(f"📚 Using: {self.clang} {self.flags}")
        print("-" * 60)
        
        files = self.find_cpp_files(root)
        
        if not files:
            print("No C++ files found!")
            return True
        
        print(f"Found {len(files)} files to check...\n")
        
        # Check files in parallel
        success_count = 0
        fail_count = 0
        
        with ThreadPoolExecutor(max_workers=workers) as executor:
            future_to_file = {
                executor.submit(self.check_file, file): file 
                for file in files
            }
            
            for future in as_completed(future_to_file):
                file = future_to_file[future]
                try:
                    success, message = future.result()
                    print(message)
                    
                    if success:
                        success_count += 1
                    else:
                        fail_count += 1
                        self.errors.append(f"{file}: {message}")
                        
                except Exception as e:
                    print(f"⚠️  {file.name} (exception: {str(e)})")
                    fail_count += 1
        
        # Summary
        print("\n" + "=" * 60)
        print(f"📊 Summary:")
        print(f"  Total files: {len(files)}")
        print(f"  ✅ Passed: {success_count}")
        print(f"  ❌ Failed: {fail_count}")
        
        if self.errors and fail_count > 0:
            print("\n🔍 Errors found:")
            for error in self.errors[:10]:  # Show first 10 errors
                print(f"  - {error}")
            if len(self.errors) > 10:
                print(f"  ... and {len(self.errors) - 10} more")
        
        return fail_count == 0

def main():
    parser = argparse.ArgumentParser(description="C++ syntax checker similar to cargo check")
    parser.add_argument("path", nargs="?", default=".", help="Path to check (default: current directory)")
    parser.add_argument("--clang", default="clang++", help="Clang++ compiler path (default: clang++)")
    parser.add_argument("--flags", default="-std=c++17 -Wall -Wextra -I.", 
                       help="Compiler flags (default: -std=c++17 -Wall -Wextra -I.)")
    parser.add_argument("--workers", type=int, default=4, 
                       help="Number of parallel workers (default: 4)")
    parser.add_argument("--list", action="store_true", 
                       help="List files to check without checking")
    
    args = parser.parse_args()
    
    checker = CppSyntaxChecker(args.clang, args.flags)
    
    if args.list:
        root = Path(args.path).resolve()
        files = checker.find_cpp_files(root)
        print(f"Found {len(files)} C++ files:")
        for file in sorted(files):
            print(f"  {file.relative_to(root)}")
        return 0
    
    success = checker.run(args.path, args.workers)
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())
