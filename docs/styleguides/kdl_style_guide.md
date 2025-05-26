# KDL Style Guide

This document contains a set of guidelines for writing KDL documents in Harvest.

## Table of Contents

- [General](#general)
- [Whitespace and Braces](#whitespace-and-braces)
- [Comments](#comments)
- [Files](#files)
- [Types](#types)
    * [Strings](#strings)

## General

- Follow the [KDL v2.0.0 Spec](https://github.com/kdl-org/kdl/blob/2.0.0/SPEC.md)

## Whitespace and Braces

- All indentation should use 4 spaces
- Use Allman style braces, which puts open and close braces on their own line

## Comments

- Comments have a hard length max at column 100
- Prefer single-line comments (`//`) to multi-line comments (`/* */`)
- Use separator comments (`// -------------`) that end at column 100 to section off logically related data

## Files

- File names should be `lower_snake_case` and use a `.kdl` extension
    * E.g.: `he_project.kdl`
- Start all files with a simple copyright comment as their first line (`// Copyright Chad Engler`)

## Types

### Strings

- Avoid multiline strings (`""" """`) if possible
