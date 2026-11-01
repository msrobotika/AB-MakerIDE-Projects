@echo off
cd /d "%~dp0"
py -3 ghostguard_agent.py --port COM4
pause
