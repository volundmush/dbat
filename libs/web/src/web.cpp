#include "web/web.hpp"

#include <algorithm>

namespace dbat::web {
	namespace {
		void split_path(std::string_view path, std::vector<std::string_view>& out) {
			out.clear();
			if (path.empty()) {
				return;
			}

			while (!path.empty() && path.front() == '/') {
				path.remove_prefix(1);
			}
			while (!path.empty() && path.back() == '/') {
				path.remove_suffix(1);
			}
			if (path.empty()) {
				return;
			}

			size_t start = 0;
			while (start < path.size()) {
				auto pos = path.find('/', start);
				if (pos == std::string_view::npos) {
					out.emplace_back(path.substr(start));
					break;
				}
				out.emplace_back(path.substr(start, pos - start));
				start = pos + 1;
			}
		}

		bool is_param(std::string_view segment) {
			return !segment.empty() && segment.front() == ':';
		}
	}

	void Router::add(std::string_view path, http::verb method, Handler handler) {
		std::vector<std::string_view> segments;
		split_path(path, segments);

		Node* node = &root_;
		for (auto segment : segments) {
			if (is_param(segment)) {
				if (!node->param_child) {
					node->param_child = std::make_unique<Node>();
					node->param_name = std::string(segment.substr(1));
				}
				node = node->param_child.get();
			} else {
				auto& child = node->static_children[std::string(segment)];
				if (!child) {
					child = std::make_unique<Node>();
				}
				node = child.get();
			}
		}

		node->handlers[method] = std::move(handler);
	}

	boost::asio::awaitable<std::optional<Result>> Router::dispatch(RouteContext& ctx) const {
		std::vector<std::string_view> segments;
		split_path(ctx.url.path(), segments);

		const Node* node = &root_;
		Params params;

		for (auto segment : segments) {
			auto it = node->static_children.find(std::string(segment));
			if (it != node->static_children.end()) {
				node = it->second.get();
				continue;
			}

			if (node->param_child) {
				params[node->param_name] = std::string(segment);
				node = node->param_child.get();
				continue;
			}

			co_return std::nullopt;
		}

		auto hit = node->handlers.find(ctx.req.method());
		if (hit == node->handlers.end()) {
			co_return std::optional<Result>{std::unexpected(HttpError{http::status::method_not_allowed, "Method Not Allowed\n"})};
		}

		ctx.params = std::move(params);
		auto result = co_await hit->second(ctx);
		co_return std::optional<Result>{std::move(result)};
	}
}
