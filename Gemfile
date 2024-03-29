# frozen_string_literal: true

source "https://rubygems.org"

git_source(:github) do |repo_name|
  repo_name = "#{repo_name}/#{repo_name}" unless repo_name.include?("/")
  "https://github.com/#{repo_name}.git"
end

ruby "2.6.2"
# Use "actionview" for rendering
gem "actionview", ">= 5.1.6.2"
# Use "bootstrap" for CSS styling
gem "bootstrap", ">= 4.3.1"
gem "bundler", ">= 1.17"
# Use CoffeeScript for .coffee assets and views
gem "coffee-rails", "~> 4.2"
gem "haml"
gem "loofah", ">= 2.2.3"
# Build JSON APIs with ease. Read more: https://github.com/rails/jbuilder
gem "jbuilder", "~> 2.5"
gem "jquery-rails"
# Use Puma as the app server
gem "puma"
gem "rack", ">= 2.0.6"
# Bundle edge Rails instead: gem 'rails', github: 'rails/rails'
gem "rails", "~> 5.1.6"
# Use SCSS for stylesheets
gem "sass-rails", "~> 5.0"
# Turbolinks makes navigating your web application faster. Read more: https://github.com/turbolinks/turbolinks
gem "turbolinks", "~> 5"
# Use Uglifier as compressor for JavaScript assets
gem "uglifier", ">= 1.3.0"
# gem "webpacker"
# See https://github.com/rails/execjs#readme for more supported runtimes
# gem "therubyracer", platforms: :ruby
# Use Capistrano for deployment
# gem "capistrano-rails", group: :development


group :development, :test do
  gem "better_errors"
  # Call "byebug" anywhere in the code to stop execution and get a debugger console
  gem "byebug", platforms: %i(mri mingw x64_mingw)
  # Adds support for Capybara system testing and selenium driver
  gem "capybara", "~> 2.13"
  gem "selenium-webdriver"
  # Use sqlite3 as the database for Active Record
  gem "sqlite3"
end

group :development do
  gem "listen", ">= 3.0.5", "< 3.2"
  # Spring speeds up development by keeping your application running in the background. Read more: https://github.com/rails/spring
  gem "spring"
  gem "spring-watcher-listen", "~> 2.0.0"
  # Access an IRB console on exception pages or by using <%= console %> anywhere in the code.
  gem "web-console", ">= 3.3.0"
end

group :production do
  gem "pg"
  gem "rails_12factor"
end

# Windows does not include zoneinfo files, so bundle the tzinfo-data gem
gem "tzinfo-data", platforms: %i(mingw mswin x64_mingw jruby)
